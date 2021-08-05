/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-fragment-host.h"

#include "gpropz.h"

G_DEFINE_QUARK("grex-fragment-host-on-widget", grex_fragment_host_on_widget)
#define GREX_FRAGMENT_HOST_ON_WIDGET (grex_fragment_host_on_widget_quark())

struct _GrexFragmentHost {
  GObject parent_instance;

  GrexPropertySet *applied_properties;
  GWeakRef widget;
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
 * grex_fragment_host_create_with_widget:
 * @fragment: The fragment to base this host on.
 *
 * Creates a new #GtkWidget and #GrexFragmentHost, using the widget type and
 * properties located within the given fragment.
 *
 * Returns: (transfer full): A new widget, containing a new fragment host.
 */
GtkWidget *
grex_fragment_host_create_with_widget(GrexFragment *fragment) {
  GType widget_type = grex_fragment_get_widget_type(fragment);
  GtkWidget *widget = GTK_WIDGET(g_object_new(widget_type, NULL));
  grex_fragment_host_new(widget);
  return widget;
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
    g_auto(GValue) value = G_VALUE_INIT;

    g_warn_if_fail(grex_property_set_get(properties, name, &value));
    g_object_set_property(widget_object, name, &value);

    grex_property_set_add(host->applied_properties, name, &value);
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
