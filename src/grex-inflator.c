/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-inflator.h"

#include "gpropz.h"
#include "grex-fragment-host.h"
#include "grex-property-set.h"

struct _GrexInflator {
  GObject parent_instance;

  GrexExpressionContext *context;

  GHashTable *directive_factories;
  GHashTable *auto_directive_names;
};

enum {
  PROP_CONTEXT = 1,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexInflator, grex_inflator, G_TYPE_OBJECT)

static void
grex_inflator_finalize(GObject *object) {
  GrexInflator *inflator = GREX_INFLATOR(object);

  g_clear_pointer(&inflator->auto_directive_names,  // NOLINT
                  g_hash_table_unref);
  g_clear_pointer(&inflator->directive_factories,  // NOLINT
                  g_hash_table_unref);
}

static void
grex_inflator_class_init(GrexInflatorClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = grex_inflator_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_CONTEXT] = g_param_spec_object(
      "context", "Evaluation context",
      "The context used to evaluation expressions.",
      GREX_TYPE_EXPRESSION_CONTEXT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexInflator, context, PROP_CONTEXT,
                          properties[PROP_CONTEXT], NULL);
}

static void
grex_inflator_init(GrexInflator *inflator) {
  inflator->directive_factories =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
  inflator->auto_directive_names = g_hash_table_new(g_str_hash, g_str_equal);
}

/**
 * grex_inflator_new:
 * @context: The context used to evaluate expressions.
 *
 * Creates a new #GrexInflator.
 *
 * Returns: (transfer full): A new inflator.
 */
GrexInflator *
grex_inflator_new(GrexExpressionContext *context) {
  return g_object_new(GREX_TYPE_INFLATOR, "context", context, NULL);
}

/**
 * grex_inflator_new_with_scope:
 * @scope: The scope to look up expression values in.
 *
 * Creates a new #GrexInflator, creating a new #GrexExpressionContext with the
 * given scope in the process.
 *
 * Returns: (transfer full): A new inflator.
 */
GrexInflator *
grex_inflator_new_with_scope(GObject *scope) {
  g_autoptr(GrexExpressionContext) context = grex_expression_context_new();
  grex_expression_context_add_scope(context, scope);
  return grex_inflator_new(context);
}

/**
 * grex_inflator_get_context:
 *
 * Returns the expression context used to evaluate bindings.
 *
 * Returns: (transfer none): The expression context.
 */
GPROPZ_DEFINE_RO(GrexExpressionContext *, GrexInflator, grex_inflator, context,
                 properties[PROP_CONTEXT])

void
grex_inflator_take_directives(GrexInflator *inflator,
                              GrexInflatorDirectiveFlags flags, ...) {
  g_autoptr(GPtrArray) directives =
      g_ptr_array_new_with_free_func(g_object_unref);

  va_list va;
  va_start(va, flags);
  for (;;) {
    GrexDirectiveFactory *factory = va_arg(va, GrexDirectiveFactory *);
    if (factory == NULL) {
      break;
    }

    g_ptr_array_add(directives, factory);
  }
  va_end(va);

  grex_inflator_add_directivesv(inflator, flags, directives->len,
                                (GrexDirectiveFactory **)directives->pdata);
}

void
grex_inflator_add_directives(GrexInflator *inflator,
                             GrexInflatorDirectiveFlags flags, ...) {
  g_autoptr(GPtrArray) directives = g_ptr_array_new();

  va_list va;
  va_start(va, flags);
  for (;;) {
    GrexDirectiveFactory *factory = va_arg(va, GrexDirectiveFactory *);
    if (factory == NULL) {
      break;
    }

    g_ptr_array_add(directives, factory);
  }
  va_end(va);

  grex_inflator_add_directivesv(inflator, flags, directives->len,
                                (GrexDirectiveFactory **)directives->pdata);
}

/**
 * grex_inflator_add_directivesv: (rename-to grex_inflator_add_directives)
 * @directives: (array length=n_directives) (transfer none)
 */
void
grex_inflator_add_directivesv(GrexInflator *inflator,
                              GrexInflatorDirectiveFlags flags,
                              guint n_directives,
                              GrexDirectiveFactory **directives) {
  for (guint i = 0; i < n_directives; i++) {
    GrexDirectiveFactory *factory = directives[i];

    char *name = g_strdup(grex_directive_factory_get_name(factory));
    g_hash_table_insert(inflator->directive_factories, name,
                        g_object_ref(factory));

    if (!(flags & GREX_INFLATOR_DIRECTIVE_NO_AUTO_ATTACH)) {
      g_hash_table_add(inflator->auto_directive_names, name);
    }
  }
}

static GrexPropertySet *
grex_inflator_evaluate_fragment_property_set(GrexInflator *inflator,
                                             GrexFragment *fragment,
                                             gboolean track_dependencies) {
  g_autoptr(GrexPropertySet) properties = grex_property_set_new();
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);

  for (GList *target = targets; target != NULL; target = target->next) {
    if (g_hash_table_contains(inflator->directive_factories, target->data)) {
      // Skip it, it's a directive that is handled separately.
      continue;
    }

    g_autoptr(GError) error = NULL;
    GrexBinding *binding = grex_fragment_get_binding(fragment, target->data);

    g_auto(GValue) value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_autoptr(GrexValueHolder) result = grex_binding_evaluate(
        binding, inflator->context, track_dependencies, &error);
    if (result == NULL) {
      GrexSourceLocation *location = grex_fragment_get_location(fragment);
      g_autofree char *location_string = grex_source_location_format(location);
      g_warning("%s: Failed to evaluate binding: %s", location_string,
                error->message);
      continue;
    }

    grex_property_set_insert(properties, target->data, result);
  }

  return g_steal_pointer(&properties);
}

static void
add_directive(GrexFragmentHost *host, GrexDirectiveFactory *factory,
              GHashTable *inserted_directives) {
  g_autoptr(GrexAttributeDirective) directive =
      grex_fragment_host_get_leftover_attribute_directive(host,
                                                          (guintptr)factory);
  if (directive != NULL) {
    g_object_ref(directive);
  } else {
    directive =
        GREX_ATTRIBUTE_DIRECTIVE(grex_directive_factory_create(factory));
  }

  grex_fragment_host_add_attribute_directive(host, (guintptr)factory,
                                             directive);
  grex_fragment_host_apply_pending_directive_updates(host);

  g_hash_table_add(inserted_directives, factory);
}

static void
grex_inflator_auto_attach_directives(GrexInflator *inflator,
                                     GrexFragmentHost *host,
                                     GrexFragment *fragment,
                                     GHashTable *inserted_directives) {
  GHashTableIter iter;
  gpointer key;

  g_hash_table_iter_init(&iter, inflator->auto_directive_names);
  while (g_hash_table_iter_next(&iter, &key, NULL)) {
    const char *name = key;
    GrexDirectiveFactory *factory =
        g_hash_table_lookup(inflator->directive_factories, name);
    if (G_UNLIKELY(factory == NULL)) {
      g_warning("Missing auto-assign directive: %s", name);
      continue;
    }

    if (!g_hash_table_contains(inserted_directives, factory) &&
        grex_directive_factory_should_auto_attach(factory, host, fragment)) {
      add_directive(host, factory, inserted_directives);
    }
  }
}

static void
grex_inflator_apply_directives(GrexInflator *inflator, GrexFragmentHost *host,
                               GrexFragment *fragment) {
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);
  g_autoptr(GHashTable) inserted_directives =
      g_hash_table_new(g_str_hash, g_str_equal);

  for (GList *target = targets; target != NULL; target = target->next) {
    const char *name = target->data;
    GrexDirectiveFactory *factory =
        g_hash_table_lookup(inflator->directive_factories, name);
    if (factory == NULL) {
      // Not a directive.
      continue;
    }

    add_directive(host, factory, inserted_directives);
  }

  grex_inflator_auto_attach_directives(inflator, host, fragment,
                                       inserted_directives);
}

/**
 * grex_inflator_inflate_new_target:
 * @fragment: (transfer none): The fragment to inflate.
 *
 * Creates a new #GObject, inflating the given fragment into it.
 *
 * Returns: (transfer full): The newly created object.
 */
GObject *
grex_inflator_inflate_new_target(GrexInflator *inflator, GrexFragment *fragment,
                                 GrexInflationFlags flags) {
  GType target_type = grex_fragment_get_target_type(fragment);

  // TODO: handle construct-only properties.
  GObject *target = g_object_new(target_type, NULL);
  grex_inflator_inflate_existing_target(inflator, target, fragment, flags);
  return target;
}

/**
 * grex_inflator_inflate_existing_object:
 * @fragment: (transfer none): The fragment to inflate.
 * @target: (transfer none): The object to inflate the fragent into.
 *
 * Inflates the given fragment into the given object.
 */
void
grex_inflator_inflate_existing_target(GrexInflator *inflator, GObject *target,
                                      GrexFragment *fragment,
                                      GrexInflationFlags flags) {
  g_autoptr(GrexFragmentHost) host = grex_fragment_host_for_target(target);
  if (host == NULL) {
    host = grex_fragment_host_new(target);
  } else {
    g_object_ref(host);
    g_return_if_fail(grex_fragment_host_matches_fragment_type(host, fragment));
  }

  g_autoptr(GrexPropertySet) properties =
      grex_inflator_evaluate_fragment_property_set(
          inflator, fragment, flags & GREX_INFLATION_TRACK_DEPENDENCIES);
  grex_fragment_host_apply_latest_properties(host, properties);

  grex_fragment_host_begin_inflation(host);

  grex_inflator_apply_directives(inflator, host, fragment);

  g_autoptr(GList) children = grex_fragment_get_children(fragment);
  for (GList *child = children; child != NULL; child = child->next) {
    grex_inflator_inflate_child(inflator, host, (guintptr)child->data,
                                child->data, flags);
  }

  grex_fragment_host_commit_inflation(host);
}

void
grex_inflator_inflate_child(GrexInflator *inflator, GrexFragmentHost *parent,
                            guintptr key, GrexFragment *child,
                            GrexInflationFlags flags) {
  GObject *child_object = grex_fragment_host_get_leftover_child(parent, key);
  if (child_object == NULL) {
    child_object = grex_inflator_inflate_new_target(inflator, child, flags);
  } else {
    grex_inflator_inflate_existing_target(inflator, child_object, child, flags);
  }

  grex_fragment_host_add_inflated_child(parent, key, child_object);
}
