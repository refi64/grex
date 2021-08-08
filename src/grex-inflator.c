/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-inflator.h"

#include "grex-fragment-host.h"
#include "grex-property-set.h"

struct _GrexInflator {
  GObject parent_instance;
};

G_DEFINE_TYPE(GrexInflator, grex_inflator, G_TYPE_OBJECT)

static void
grex_inflator_class_init(GrexInflatorClass *klass) {}

static void
grex_inflator_init(GrexInflator *inflator) {}

/**
 * grex_inflator_new:
 *
 * Creates a new #GrexInflator.
 *
 * Returns: (transfer full): A new inflator.
 */
GrexInflator *
grex_inflator_new() {
  return g_object_new(GREX_TYPE_INFLATOR, NULL);
}

static GrexPropertySet *
grex_inflator_evaluate_fragment_property_set(GrexFragment *fragment) {
  g_autoptr(GrexPropertySet) properties = grex_property_set_new();
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);

  for (GList *target = targets; target != NULL; target = target->next) {
    g_autoptr(GError) error = NULL;
    GrexBinding *binding = grex_fragment_get_binding(fragment, target->data);

    g_auto(GValue) value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_autoptr(GrexValueHolder) result = grex_binding_evaluate(binding, &error);
    if (result == NULL) {
      GrexSourceLocation *location = grex_binding_get_location(binding);
      g_autofree char *location_string = grex_source_location_format(location);
      g_warning("%s: Failed to evaluate binding: %s", location_string,
                error->message);
      continue;
    }

    grex_property_set_insert(properties, target->data, result);
  }

  return g_steal_pointer(&properties);
}

/**
 * grex_inflator_inflate_new_widget:
 * @fragment: (transfer none): The fragment to inflate.
 *
 * Creates a new #GtkWidget, inflating the given fragment into it.
 *
 * Returns: (transfer full): The newly created widget.
 */
GtkWidget *
grex_inflator_inflate_new_widget(GrexInflator *inflator,
                                 GrexFragment *fragment) {
  GType widget_type = grex_fragment_get_widget_type(fragment);
  g_return_val_if_fail(g_type_is_a(widget_type, GTK_TYPE_WIDGET), NULL);

  // TODO: handle construct-only properties.
  GtkWidget *widget = GTK_WIDGET(g_object_new(widget_type, NULL));
  grex_inflator_inflate_existing_widget(inflator, widget, fragment);
  return widget;
}

/**
 * grex_inflator_inflate_existing_widget:
 * @fragment: (transfer none): The fragment to inflate.
 * @widget: (transfer none): The widget to inflate the fragent into.
 *
 * Inflates the given fragment into the given widget.
 */
void
grex_inflator_inflate_existing_widget(GrexInflator *inflator, GtkWidget *widget,
                                      GrexFragment *fragment) {
  g_autoptr(GrexFragmentHost) host = grex_fragment_host_for_widget(widget);
  if (host == NULL) {
    host = grex_fragment_host_new(widget);
  } else {
    g_object_ref(host);
    g_return_if_fail(grex_fragment_host_matches_fragment_type(host, fragment));
  }

  g_autoptr(GrexPropertySet) properties =
      grex_inflator_evaluate_fragment_property_set(fragment);
  grex_fragment_host_apply_latest_properties(host, properties);
}
