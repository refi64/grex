/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-fragment-host.h"

#include "gpropz.h"
#include "grex-property-directive.h"
#include "grex-value-parser.h"

G_DEFINE_QUARK("grex-fragment-host-on-target", grex_fragment_host_on_target)
#define GREX_FRAGMENT_HOST_ON_TARGET (grex_fragment_host_on_target_quark())

// Manages inflation diffs for a table of values.
typedef struct {
  // The values from the previous inflation that have not been added to the
  // current inflation.
  GHashTable *leftovers;
  // The values set in the most recently committed inflation. (This value is
  // updated at the end of an inflation and preserved until the next one
  // begins.)
  GHashTable *current;
} IncrementalTableDiff;

static void
incremental_table_diff_init(IncrementalTableDiff *diff, GHashFunc hash_func,
                            GEqualFunc equal_func,
                            GDestroyNotify key_destroy_func,
                            GDestroyNotify value_destroy_func) {
  diff->leftovers = g_hash_table_new_full(hash_func, equal_func,
                                          key_destroy_func, value_destroy_func);
  diff->current = g_hash_table_new_full(hash_func, equal_func, key_destroy_func,
                                        value_destroy_func);
}

static void
incremental_table_diff_begin_inflation(IncrementalTableDiff *diff) {
  g_return_if_fail(g_hash_table_size(diff->leftovers) == 0);

  GHashTable *leftovers = g_steal_pointer(&diff->leftovers);
  GHashTable *current = g_steal_pointer(&diff->current);

  diff->leftovers = current;
  // 'leftovers' should have been cleared when the last inflation ended, so this
  // is the same as setting it to a new GHashTable, except we don't have to
  // re-create it.
  diff->current = leftovers;
}

static gpointer
incremental_table_diff_get_leftover_value(IncrementalTableDiff *diff,
                                          guintptr key) {
  return g_hash_table_lookup(diff->leftovers, (gpointer)key);
}

static gboolean
incremental_table_diff_is_in_current_inflation(IncrementalTableDiff *diff,
                                               guintptr key) {
  return g_hash_table_contains(diff->current, (gpointer)key);
}

static void
incremental_table_diff_add_to_current_inflation(IncrementalTableDiff *diff,
                                                guintptr key, gpointer value) {
  g_hash_table_remove(diff->leftovers, (gpointer)key);
  g_hash_table_insert(diff->current, (gpointer)key, value);
}

typedef void (*IncrementalTableDiffRemovalCallback)(gpointer key,
                                                    gpointer value,
                                                    gpointer user_data);

static void
incremental_table_diff_commit_inflation(
    IncrementalTableDiff *diff, IncrementalTableDiffRemovalCallback callback,
    gpointer user_data) {
  // Remove all the values that weren't added in this inflation.

  if (callback != NULL) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, diff->leftovers);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
      callback(key, value, user_data);
    }
  }

  g_hash_table_remove_all(diff->leftovers);
}

static void
incremental_table_diff_clear(IncrementalTableDiff *diff) {
  g_clear_pointer(&diff->leftovers, g_hash_table_unref);  // NOLINT
  g_clear_pointer(&diff->current, g_hash_table_unref);    // NOLINT
}

struct _GrexFragmentHost {
  GObject parent_instance;

  GWeakRef target;
  GrexContainerAdapter *container_adapter;

  gboolean in_inflation;

  // Everything below is inflation-related state:

  // The last child added to this inflation.
  GObject *last_child;

  IncrementalTableDiff property_diff;
  IncrementalTableDiff prop_directive_diff;
  GList *pending_prop_directive_updates;
  IncrementalTableDiff struct_directive_diff;
  IncrementalTableDiff children_diff;
};

enum {
  PROP_APPLIED_PROPERTIES = 1,
  PROP_TARGET,
  PROP_CONTAINER_ADAPTER,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexFragmentHost, grex_fragment_host, G_TYPE_OBJECT)

static void
grex_fragment_host_constructed(GObject *object) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(object);

  GObject *target = grex_fragment_host_get_target(host);
  g_return_if_fail(target != NULL);
  g_object_set_qdata_full(target, GREX_FRAGMENT_HOST_ON_TARGET,
                          g_object_ref_sink(host), g_object_unref);
}

static void
detach_directive(GrexFragmentHost *host, GrexPropertyDirective *directive) {
  GrexPropertyDirectiveClass *directive_class =
      GREX_PROPERTY_DIRECTIVE_GET_CLASS(directive);
  directive_class->detach(directive, host);
}

static void
detach_directives_in_table(GrexFragmentHost *host, GHashTable *table) {
  GHashTableIter iter;
  gpointer directive;
  g_hash_table_iter_init(&iter, table);
  while (g_hash_table_iter_next(&iter, NULL, &directive)) {
    detach_directive(host, GREX_PROPERTY_DIRECTIVE(directive));
  }
}

static void
grex_fragment_host_dispose(GObject *object) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(object);
  g_clear_object(&host->container_adapter);  // NOLINT

  detach_directives_in_table(host, host->prop_directive_diff.current);
  detach_directives_in_table(host, host->prop_directive_diff.leftovers);

  incremental_table_diff_clear(&host->property_diff);
  incremental_table_diff_clear(&host->prop_directive_diff);
  g_clear_pointer(&host->pending_prop_directive_updates,  // NOLINT
                  g_list_free);
  incremental_table_diff_clear(&host->struct_directive_diff);
  incremental_table_diff_clear(&host->children_diff);
}

static void
grex_fragment_host_finalize(GObject *object) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(object);
  g_weak_ref_clear(&host->target);
}

static void
grex_fragment_host_weak_ref_filter_set(GObject *object, gpointer prop,
                                       guint prop_id, gconstpointer source,
                                       GParamSpec *pspec) {
  g_autoptr(GObject) source_object = *(GObject **)source;
  GWeakRef *prop_weak_ref = (GWeakRef *)prop;
  g_weak_ref_set(prop_weak_ref, source_object);
}

static void
grex_fragment_host_weak_ref_filter_get(GObject *object, gconstpointer prop,
                                       guint prop_id, gpointer target,
                                       GParamSpec *pspec) {
  GObject **target_object = (GObject **)target;
  GWeakRef *prop_weak_ref = (GWeakRef *)prop;
  *target_object = g_weak_ref_get(prop_weak_ref);
}

static GpropzValueFilter weak_ref_filter = {
    .get_filter = grex_fragment_host_weak_ref_filter_get,
    .set_filter = grex_fragment_host_weak_ref_filter_set,
};

static void
grex_fragment_host_class_init(GrexFragmentHostClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->constructed = grex_fragment_host_constructed;
  object_class->dispose = grex_fragment_host_dispose;
  object_class->finalize = grex_fragment_host_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_TARGET] = g_param_spec_object(
      "target", "Target",
      "The target object this fragment host is controlling.", G_TYPE_OBJECT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragmentHost, target, PROP_TARGET,
                          properties[PROP_TARGET], &weak_ref_filter);

  properties[PROP_CONTAINER_ADAPTER] = g_param_spec_object(
      "container-adapter", "Container adapter",
      "The container adapter used to add children to this object.",
      GREX_TYPE_CONTAINER_ADAPTER, G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragmentHost, container_adapter,
                          PROP_CONTAINER_ADAPTER,
                          properties[PROP_CONTAINER_ADAPTER], NULL);
}

static void
grex_fragment_host_init(GrexFragmentHost *host) {
  g_weak_ref_init(&host->target, NULL);

  incremental_table_diff_init(&host->property_diff, g_str_hash, g_str_equal,
                              g_free, (GDestroyNotify)grex_value_holder_unref);
  incremental_table_diff_init(&host->prop_directive_diff, g_direct_hash,
                              g_direct_equal, NULL, g_object_unref);
  incremental_table_diff_init(&host->struct_directive_diff, g_direct_hash,
                              g_direct_equal, NULL, g_object_unref);
  incremental_table_diff_init(&host->children_diff, g_direct_hash,
                              g_direct_equal, NULL, g_object_unref);
}

/**
 * grex_fragment_host_new:
 * @target: The target owning this fragment host.
 *
 * Creates a new #GrexFragmentHost, owned by the given target.
 *
 * Returns: (transfer full): A new fragment host.
 */
GrexFragmentHost *
grex_fragment_host_new(GObject *target) {
  return g_object_new(GREX_TYPE_FRAGMENT_HOST, "target", target, NULL);
}

/**
 * grex_fragment_host_for_target:
 * @target: The target object.
 *
 * Locates the given object's owned fragment host and returns it.
 *
 * Returns: (transfer none): The object's fragment host, or NULL if none is
 *                           present.
 */
GrexFragmentHost *
grex_fragment_host_for_target(GObject *target) {
  return g_object_get_qdata(target, GREX_FRAGMENT_HOST_ON_TARGET);
}

/**
 * grex_fragment_host_get_container_adapter:
 *
 * Returns the #GrexContainerAdapter used to add children to this object.
 *
 * Returns: (transfer none) (allow-none): The container adapter.
 */

/**
 * grex_fragment_host_set_container_adapter:
 * @adapter: (transfer none) (allow-none): The new container adapter.
 *
 * Sets #GrexContainerAdapter used to add children to this object.
 */
GPROPZ_DEFINE_RW(GrexContainerAdapter *, GrexFragmentHost, grex_fragment_host,
                 container_adapter, properties[PROP_CONTAINER_ADAPTER])

/**
 * grex_fragment_host_get_target:
 *
 * Returns the #GObject that owns and is controlled by this fragment host.
 *
 * Returns: (transfer none): The object.
 */
GPROPZ_DEFINE_RO(GObject *, GrexFragmentHost, grex_fragment_host, target,
                 properties[PROP_TARGET])

/**
 * grex_fragment_host_matches_fragment_type:
 * @fragment: The fragment whose type to check.
 *
 * Determines whether or not the type of this host's target is identical to
 * the given fragment's target type.
 *
 * Returns: TRUE if the type matches.
 */
gboolean
grex_fragment_host_matches_fragment_type(GrexFragmentHost *host,
                                         GrexFragment *fragment) {
  GObject *target = grex_fragment_host_get_target(host);
  GType type = G_OBJECT_TYPE(target);
  return type == grex_fragment_get_target_type(fragment);
}

/**
 * grex_fragment_host_begin_inflation:
 *
 * Begins an inflation "transaction" for this fragment host. The set of
 * children added to the fragment host before the inflation is committed will
 * be the only children, in the correct order, once it is committed.
 */
void
grex_fragment_host_begin_inflation(GrexFragmentHost *host) {
  g_return_if_fail(!host->in_inflation);
  host->in_inflation = TRUE;

  host->last_child = NULL;

  incremental_table_diff_begin_inflation(&host->property_diff);
  incremental_table_diff_begin_inflation(&host->prop_directive_diff);
  incremental_table_diff_begin_inflation(&host->children_diff);
  incremental_table_diff_begin_inflation(&host->struct_directive_diff);
}

/**
 * grex_fragment_host_get_leftover_property_directive:
 * @key: The property directive's key.
 *
 * Finds and returns the directive with the given key from the *previous*
 * inflation call, but only if another directive with the same key has not been
 * added to the current inflation.
 *
 * May only be called during an inflation.
 *
 * Returns: (transfer none): The directive, or NULL if none was found.
 */
GrexPropertyDirective *
grex_fragment_host_get_leftover_property_directive(GrexFragmentHost *host,
                                                   guintptr key) {
  g_return_val_if_fail(host->in_inflation, NULL);
  return incremental_table_diff_get_leftover_value(&host->prop_directive_diff,
                                                   key);
}

/**
 * grex_fragment_host_get_leftover_structural_directive:
 * @key: The structural directive's key.
 *
 * Finds and returns the directive with the given key from the *previous*
 * inflation call, but only if another directive with the same key has not been
 * added to the current inflation.
 *
 * May only be called during an inflation.
 *
 * Returns: (transfer none): The directive, or NULL if none was found.
 */
GrexStructuralDirective *
grex_fragment_host_get_leftover_structural_directive(GrexFragmentHost *host,
                                                     guintptr key) {
  g_return_val_if_fail(host->in_inflation, NULL);
  return incremental_table_diff_get_leftover_value(&host->struct_directive_diff,
                                                   key);
}

/**
 * grex_fragment_host_get_leftover_child:
 * @key: The child's key.
 *
 * Finds and returns the child with the given key from the *previous*
 * inflation call, but only if another child with the same key has not been
 * added to the current inflation.
 *
 * May only be called during an inflation.
 *
 * Returns: (transfer none): The child, or NULL if none was found.
 */
GObject *
grex_fragment_host_get_leftover_child(GrexFragmentHost *host, guintptr key) {
  g_return_val_if_fail(host->in_inflation, NULL);
  return incremental_table_diff_get_leftover_value(&host->children_diff, key);
}

/**
 * grex_fragment_host_add_property:
 * @name: The property name.
 * @value: The new value.
 *
 * Adds a new property assignment to this fragment and assigns it on the target
 * object.
 *
 * May only be called during an inflation.
 */
void
grex_fragment_host_add_property(GrexFragmentHost *host, const char *name,
                                GrexValueHolder *value) {
  g_return_if_fail(host->in_inflation);

  // NOTE: We don't bother checking if this is in the current inflation, since
  // overwriting properties is an entirely valid use case.

  GObject *target = grex_fragment_host_get_target(host);
  GObjectClass *target_class = G_OBJECT_GET_CLASS(target);

  GParamSpec *pspec = g_object_class_find_property(target_class, name);
  if (pspec == NULL) {
    g_warning("Unknown property: %s", name);
    return;
  }

  const GValue *actual_value = grex_value_holder_get_value(value);
  if (g_value_type_transformable(actual_value->g_type, pspec->value_type)) {
    g_object_set_property(target, name, actual_value);
  } else if (actual_value->g_type == G_TYPE_STRING) {
    GrexValueParser *parser = grex_value_parser_default();
    g_autoptr(GError) error = NULL;

    g_autoptr(GrexValueHolder) parsed_type = grex_value_parser_try_parse(
        parser, g_value_get_string(actual_value), pspec->value_type, &error);
    if (parsed_type == NULL) {
      g_warning("Failed to parse value for '%s': %s", name, error->message);
      return;
    }

    g_object_set_property(target, name,
                          grex_value_holder_get_value(parsed_type));
  } else {
    g_warning("Cannot assign to '%s'", name);
  }

  incremental_table_diff_add_to_current_inflation(&host->property_diff,
                                                  (guintptr)g_strdup(name),
                                                  grex_value_holder_ref(value));
}

/**
 * grex_fragment_host_add_property_directive:
 * @directive: The directive.
 *
 * Adds a new property directive to this fragment host. The directive's update
 * method will not be called until
 * #grex_fragment_host_apply_pending_directive_updates, or the inflation is
 * committed.
 *
 * May only be called during an inflation.
 */
void
grex_fragment_host_add_property_directive(GrexFragmentHost *host, guintptr key,
                                          GrexPropertyDirective *directive) {
  g_return_if_fail(host->in_inflation);

  if (incremental_table_diff_is_in_current_inflation(&host->prop_directive_diff,
                                                     key)) {
    g_warning("Attempted to add directive with key '%p' twice", (gpointer)key);
    return;
  }

  if (incremental_table_diff_get_leftover_value(&host->prop_directive_diff,
                                                key) != directive) {
    GrexPropertyDirectiveClass *directive_class =
        GREX_PROPERTY_DIRECTIVE_GET_CLASS(directive);
    directive_class->attach(directive, host);
  }

  incremental_table_diff_add_to_current_inflation(&host->prop_directive_diff,
                                                  key, g_object_ref(directive));
  host->pending_prop_directive_updates =
      g_list_prepend(host->pending_prop_directive_updates, directive);
}

/**
 * grex_fragment_host_apply_pending_directive_updates:
 *
 * Applies any updates pending to inflated directives.
 */
void
grex_fragment_host_apply_pending_directive_updates(GrexFragmentHost *host) {
  g_return_if_fail(host->in_inflation);

  g_autoptr(GList) pending =
      g_steal_pointer(&host->pending_prop_directive_updates);
  for (GList *directive = pending; directive != NULL;
       directive = directive->next) {
    GrexPropertyDirective *property_directive =
        GREX_PROPERTY_DIRECTIVE(directive->data);
    GrexPropertyDirectiveClass *directive_class =
        GREX_PROPERTY_DIRECTIVE_GET_CLASS(property_directive);
    directive_class->update(property_directive, host);
  }
}

/**
 * grex_fragment_host_add_structural_directive:
 * @directive: The directive.
 *
 * Adds a new structural directive to this fragment host.
 *
 * May only be called during an inflation.
 */
void
grex_fragment_host_add_structural_directive(
    GrexFragmentHost *host, guintptr key, GrexStructuralDirective *directive) {
  g_return_if_fail(host->in_inflation);

  if (incremental_table_diff_is_in_current_inflation(
          &host->struct_directive_diff, key)) {
    g_warning("Attempted to add directive with key '%p' twice", (gpointer)key);
    return;
  }

  incremental_table_diff_add_to_current_inflation(&host->struct_directive_diff,
                                                  key, g_object_ref(directive));
}

/**
 * grex_fragment_host_add_inflated_child:
 * @key: The child's key.
 * @child: The child object.
 *
 * Inserts a new child object into this fragment host with the given key.
 *
 * May only be called during an inflation.
 */
void
grex_fragment_host_add_inflated_child(GrexFragmentHost *host, guintptr key,
                                      GObject *child) {
  g_return_if_fail(host->in_inflation);

  if (host->container_adapter == NULL) {
    g_warning("Fragment host cannot have children because it has no container "
              "adapter");
    return;
  }

  if (incremental_table_diff_is_in_current_inflation(&host->children_diff,
                                                     key)) {
    g_warning("Attempted to add child with key '%p' twice", (gpointer)key);
    return;
  }

  // Insert it after the last inserted child (or at the front if there is no
  // last child, which would mean we're still at the front).
  GObject *parent = grex_fragment_host_get_target(host);
  if (host->last_child == NULL) {
    grex_container_adapter_insert_at_front(host->container_adapter, parent,
                                           child);
  } else {
    grex_container_adapter_insert_next_to(host->container_adapter, parent,
                                          child, host->last_child);
  }
  host->last_child = child;

  incremental_table_diff_add_to_current_inflation(&host->children_diff, key,
                                                  g_object_ref(child));
}

static void
property_diff_removal_callback(gpointer name, gpointer value,
                               gpointer user_data) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(user_data);

  GObject *target = grex_fragment_host_get_target(host);
  GObjectClass *object_class = G_OBJECT_GET_CLASS(target);

  GParamSpec *pspec = g_object_class_find_property(object_class, name);
  const GValue *default_value = g_param_spec_get_default_value(pspec);
  g_object_set_property(target, name, default_value);
}

static void
child_diff_removal_callback(gpointer key, gpointer child, gpointer user_data) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(user_data);
  g_return_if_fail(host->container_adapter != NULL);

  GObject *parent = grex_fragment_host_get_target(host);

  // XXX: We should probably also do this on destroy, but will the parent
  // actually still be set because it's in a weakref?
  grex_container_adapter_remove(host->container_adapter, parent, child);
}

static void
property_directive_detach_removal_callback(gpointer key, gpointer directive,
                                           gpointer user_data) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(user_data);
  detach_directive(host, GREX_PROPERTY_DIRECTIVE(directive));
}

/**
 * grex_fragment_host_commit_inflation:
 *
 * Commits the current inflation. This also applies any outstanding directive
 * updates.
 */
void
grex_fragment_host_commit_inflation(GrexFragmentHost *host) {
  g_return_if_fail(host->in_inflation);

  grex_fragment_host_apply_pending_directive_updates(host);

  host->in_inflation = FALSE;

  incremental_table_diff_commit_inflation(&host->property_diff,
                                          property_diff_removal_callback, host);
  incremental_table_diff_commit_inflation(
      &host->prop_directive_diff, property_directive_detach_removal_callback,
      host);
  incremental_table_diff_commit_inflation(&host->struct_directive_diff, NULL,
                                          NULL);
  incremental_table_diff_commit_inflation(&host->children_diff,
                                          child_diff_removal_callback, host);
}
