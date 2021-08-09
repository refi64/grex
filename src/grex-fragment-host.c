/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-fragment-host.h"

#include "gpropz.h"

G_DEFINE_QUARK("grex-fragment-host-on-widget", grex_fragment_host_on_widget)
#define GREX_FRAGMENT_HOST_ON_WIDGET (grex_fragment_host_on_widget_quark())

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

typedef void (*IncrementalTableDiffRemovalCallback)(gpointer value,
                                                    gpointer user_data);

static void
incremental_table_diff_commit_inflation(
    IncrementalTableDiff *diff, IncrementalTableDiffRemovalCallback callback,
    gpointer user_data) {
  // Remove all the values that weren't added in this inflation.
  GHashTableIter iter;
  gpointer value;
  g_hash_table_iter_init(&iter, diff->leftovers);
  while (g_hash_table_iter_next(&iter, NULL, &value)) {
    callback(value, user_data);
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

  GrexPropertySet *applied_properties;
  GWeakRef widget;

  gboolean in_inflation;

  // Everything below is inflation-related state:

  // The last child added to this inflation.
  GtkWidget *last_child;

  IncrementalTableDiff children_diff;
};

enum {
  PROP_APPLIED_PROPERTIES = 1,
  PROP_WIDGET,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexFragmentHost, grex_fragment_host, G_TYPE_OBJECT)

static void
grex_fragment_host_constructed(GObject *object) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(object);

  GtkWidget *widget = grex_fragment_host_get_widget(host);
  g_return_if_fail(widget != NULL);
  g_object_set_qdata_full(G_OBJECT(widget), GREX_FRAGMENT_HOST_ON_WIDGET,
                          g_object_ref_sink(host), g_object_unref);
}

static void
grex_fragment_host_dispose(GObject *object) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(object);
  g_clear_object(&host->applied_properties);  // NOLINT
}

static void
grex_fragment_host_finalize(GObject *object) {
  GrexFragmentHost *host = GREX_FRAGMENT_HOST(object);
  g_weak_ref_clear(&host->widget);

  incremental_table_diff_clear(&host->children_diff);
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

  properties[PROP_APPLIED_PROPERTIES] = g_param_spec_object(
      "applied-properties", "Applied widget properties",
      "The set of properties currently applied to this host's widget.",
      GREX_TYPE_PROPERTY_SET, G_PARAM_READABLE);
  gpropz_install_property(object_class, GrexFragmentHost, applied_properties,
                          PROP_APPLIED_PROPERTIES,
                          properties[PROP_APPLIED_PROPERTIES], NULL);

  properties[PROP_WIDGET] = g_param_spec_object(
      "widget", "Widget", "The widget this fragment host is controlling.",
      GTK_TYPE_WIDGET, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragmentHost, widget, PROP_WIDGET,
                          properties[PROP_WIDGET], &weak_ref_filter);
}

static void
grex_fragment_host_init(GrexFragmentHost *host) {
  host->applied_properties = grex_property_set_new();
  g_weak_ref_init(&host->widget, NULL);

  host->last_child = NULL;

  incremental_table_diff_init(&host->children_diff, g_direct_hash,
                              g_direct_equal, NULL, g_object_unref);
}

/**
 * grex_fragment_host_new:
 * @widget: The widget owning this fragment host.
 *
 * Creates a new #GrexFragmentHost, owned by the given widget.
 *
 * Returns: (transfer full): A new fragment host.
 */
GrexFragmentHost *
grex_fragment_host_new(GtkWidget *widget) {
  return g_object_new(GREX_TYPE_FRAGMENT_HOST, "widget", widget, NULL);
}

/**
 * grex_fragment_host_for_widget:
 * @widget: The widget.
 *
 * Locates the given widget's owned fragment host and returns it.
 *
 * Returns: (transfer none): The widget's fragment host, or NULL if none is
 *                           present.
 */
GrexFragmentHost *
grex_fragment_host_for_widget(GtkWidget *widget) {
  return g_object_get_qdata(G_OBJECT(widget), GREX_FRAGMENT_HOST_ON_WIDGET);
}

/**
 * grex_fragment_host_get_applied_properties:
 *
 * Returns the #GrexPropertySet containing the properties currently applied to
 * this host's widget.
 *
 * Returns: (transfer none): The applied properties.
 */
GPROPZ_DEFINE_RO(GrexPropertySet *, GrexFragmentHost, grex_fragment_host,
                 applied_properties, properties[PROP_APPLIED_PROPERTIES])

/**
 * grex_fragment_host_get_widget:
 *
 * Returns the #GtkWidget that owns this fragment host.
 *
 * Returns: (transfer none): The widget.
 */
GPROPZ_DEFINE_RO(GtkWidget *, GrexFragmentHost, grex_fragment_host, widget,
                 properties[PROP_WIDGET])

/**
 * grex_fragment_host_matches_fragment_type:
 * @fragment: The fragment whose type to check.
 *
 * Determines whether or not the type of this host's widget is identical to the
 * given fragment's widget type.
 *
 * Returns: TRUE if the type matches.
 */
gboolean
grex_fragment_host_matches_fragment_type(GrexFragmentHost *host,
                                         GrexFragment *fragment) {
  GtkWidget *widget = grex_fragment_host_get_widget(host);
  GType type = G_OBJECT_TYPE(widget);
  return type == grex_fragment_get_widget_type(fragment);
}

static void
update_properties_by_name(GrexFragmentHost *host, GObject *widget_object,
                          GrexPropertySet *properties, GList *names) {
  for (; names != NULL; names = names->next) {
    const char *name = names->data;

    GrexValueHolder *value = grex_property_set_get(properties, name);
    g_warn_if_fail(value != NULL);
    g_object_set_property(widget_object, name,
                          grex_value_holder_get_value(value));

    grex_property_set_insert(host->applied_properties, name, value);
  }
}

/**
 * grex_fragment_host_apply_latest_properties:
 * @properties: The set of properties to apply.
 *
 * Determines the difference between the given property set and the host's
 * current applied properties (i.e. what values were added, removed, or
 * changed), and updates the widget and applied properties to match the given
 * property set.
 */
void
grex_fragment_host_apply_latest_properties(GrexFragmentHost *host,
                                           GrexPropertySet *properties) {
  g_autoptr(GList) added = NULL;
  g_autoptr(GList) removed = NULL;
  g_autoptr(GList) kept = NULL;

  GObject *widget_object = G_OBJECT(grex_fragment_host_get_widget(host));

  grex_property_set_diff_keys(host->applied_properties, properties, &added,
                              &removed, &kept);

  update_properties_by_name(host, widget_object, properties, added);
  update_properties_by_name(host, widget_object, properties, kept);

  if (removed != NULL) {
    GObjectClass *object_class = G_OBJECT_GET_CLASS(widget_object);

    for (GList *names = removed; names != NULL; names = names->next) {
      const char *name = names->data;

      GParamSpec *pspec = g_object_class_find_property(object_class, name);
      const GValue *default_value = g_param_spec_get_default_value(pspec);
      g_object_set_property(widget_object, name, default_value);

      grex_property_set_remove(host->applied_properties, name);
    }
  }
}

/**
 * grex_fragment_host_begin_inflation:
 *
 * Begins an inflation "transaction" for this fragment host. The set of children
 * added to the fragment host before the inflation is committed will be the only
 * children, in the correct order, once it is committed.
 */
void
grex_fragment_host_begin_inflation(GrexFragmentHost *host) {
  g_return_if_fail(!host->in_inflation);
  host->in_inflation = TRUE;

  host->last_child = NULL;

  incremental_table_diff_begin_inflation(&host->children_diff);
}

/**
 * grex_fragment_host_get_leftover_child_from_previous_inflation:
 * @key: The widget's key.
 *
 * Finds and returns the widget with the given key from the *previous* inflation
 * call, but only if another widget with the same key has not been added to the
 * current inflation.
 *
 * May only be called during an inflation.
 *
 * Returns: (transfer none): The widget, or NULL if none was found.
 */
GtkWidget *
grex_fragment_host_get_leftover_child_from_previous_inflation(
    GrexFragmentHost *host, guintptr key) {
  g_return_val_if_fail(host->in_inflation, NULL);
  return incremental_table_diff_get_leftover_value(&host->children_diff, key);
}

/**
 * grex_fragment_host_add_inflated_child:
 * @key: The widget's key.
 * @child: The child widget.
 *
 * Inserts a new child widget into this fragment host with the given key.
 *
 * May only be called during an inflation.
 */
void
grex_fragment_host_add_inflated_child(GrexFragmentHost *host, guintptr key,
                                      GtkWidget *child) {
  g_return_if_fail(host->in_inflation);

  if (incremental_table_diff_is_in_current_inflation(&host->children_diff,
                                                     key)) {
    g_warning("Attempted to add child with key '%p' twice", (gpointer)key);
    return;
  }

  // Insert it after the last inserted child (or at the front if there is no
  // last child, which would mean we're still at the front).
  GtkWidget *parent = grex_fragment_host_get_widget(host);
  gtk_widget_insert_after(child, parent, host->last_child);
  host->last_child = child;

  incremental_table_diff_add_to_current_inflation(&host->children_diff, key,
                                                  g_object_ref(child));
}

static void
unparent_widget_diff_removal_callback(gpointer widget, gpointer user_data) {
  gtk_widget_unparent(GTK_WIDGET(widget));
}

/**
 * grex_fragment_host_commit_inflation:
 *
 * Commits the current inflation.
 */
void
grex_fragment_host_commit_inflation(GrexFragmentHost *host) {
  g_return_if_fail(host->in_inflation);
  host->in_inflation = FALSE;

  incremental_table_diff_commit_inflation(
      &host->children_diff, unparent_widget_diff_removal_callback, NULL);
}
