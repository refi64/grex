/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-reactive-inflator.h"

#include "gpropz.h"

struct _GrexReactiveInflator {
  GObject parent_instance;

  GrexInflator *base_inflator;
  GrexFragment *fragment;
  GtkWidget *widget;
};

enum {
  PROP_BASE_INFLATOR = 1,
  PROP_FRAGMENT,
  PROP_WIDGET,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexReactiveInflator, grex_reactive_inflator, G_TYPE_OBJECT)

static void
on_context_changed(GrexExpressionContext *context, gpointer user_data) {
  GrexReactiveInflator *inflator = GREX_REACTIVE_INFLATOR(user_data);
  grex_reactive_inflator_inflate(inflator);
}

static void
grex_reactive_inflator_dispose(GObject *object) {
  GrexReactiveInflator *inflator = GREX_REACTIVE_INFLATOR(object);

  g_clear_object(&inflator->base_inflator);  // NOLINT
  g_clear_object(&inflator->fragment);       // NOLINT
  g_clear_object(&inflator->widget);         // NOLINT
}

static void
grex_reactive_inflator_constructed(GObject *object) {
  GrexReactiveInflator *inflator = GREX_REACTIVE_INFLATOR(object);

  GrexExpressionContext *context =
      grex_inflator_get_context(inflator->base_inflator);
  g_signal_connect_object(context, "changed", G_CALLBACK(on_context_changed),
                          inflator, 0);
}

static void
grex_reactive_inflator_class_init(GrexReactiveInflatorClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->constructed = grex_reactive_inflator_constructed;
  object_class->dispose = grex_reactive_inflator_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_BASE_INFLATOR] = g_param_spec_object(
      "base-inflator", "Base inflator",
      "The base inflator used to inflate the target widget.",
      GREX_TYPE_INFLATOR, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexReactiveInflator, base_inflator,
                          PROP_BASE_INFLATOR, properties[PROP_BASE_INFLATOR],
                          NULL);

  properties[PROP_FRAGMENT] = g_param_spec_object(
      "fragment", "Fragment", "The fragment to inflate.", GREX_TYPE_FRAGMENT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexReactiveInflator, fragment,
                          PROP_FRAGMENT, properties[PROP_FRAGMENT], NULL);

  properties[PROP_WIDGET] = g_param_spec_object(
      "widget", "Target widget", "The widget to inflate the fragment into.",
      GTK_TYPE_WIDGET, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexReactiveInflator, widget,
                          PROP_WIDGET, properties[PROP_WIDGET], NULL);
}

static void
grex_reactive_inflator_init(GrexReactiveInflator *inflator) {}

/**
 * grex_reactive_inflator_new:
 * @fragment: The fragment to inflate.
 * @widget: The target widget.
 *
 * Creates a new reactive inflator.
 *
 * Returns: (transfer full): The reactive inflator.
 */
GrexReactiveInflator *
grex_reactive_inflator_new(GrexFragment *fragment, GtkWidget *widget) {
  g_autoptr(GrexInflator) base_inflator =
      grex_inflator_new_with_scope(G_OBJECT(widget));
  return grex_reactive_inflator_new_with_base_inflator(base_inflator, fragment,
                                                       widget);
}

/**
 * grex_reactive_inflator_new_with_base_inflator:
 * @base_inflator: The inflator to use to inflate the fragment.
 * @fragment: The fragment to inflate.
 * @widget: The target widget.
 *
 * Creates a new reactive inflator.
 *
 * Returns: (transfer full): The reactive inflator.
 */
GrexReactiveInflator *
grex_reactive_inflator_new_with_base_inflator(GrexInflator *base_inflator,
                                              GrexFragment *fragment,
                                              GtkWidget *widget) {
  return g_object_new(GREX_TYPE_REACTIVE_INFLATOR, "base-inflator",
                      base_inflator, "fragment", fragment, "widget", widget,
                      NULL);
}

/**
 * grex_reactive_inflator_get_base_inflator:
 *
 * Returns the inflator's base inflator.
 *
 * Returns: (transfer none): The base inflator.
 */
GPROPZ_DEFINE_RO(GrexInflator *, GrexReactiveInflator, grex_reactive_inflator,
                 base_inflator, properties[PROP_BASE_INFLATOR])

/**
 * grex_reactive_inflator_get_fragment:
 *
 * Returns the inflator's fragment.
 *
 * Returns: (transfer none): The fragment.
 */
GPROPZ_DEFINE_RO(GrexFragment *, GrexReactiveInflator, grex_reactive_inflator,
                 fragment, properties[PROP_FRAGMENT])

/**
 * grex_reactive_inflator_get_widget:
 *
 * Returns the inflator's widget.
 *
 * Returns: (transfer none): The widget.
 */
GPROPZ_DEFINE_RO(GtkWidget *, GrexReactiveInflator, grex_reactive_inflator,
                 widget, properties[PROP_WIDGET])

/**
 * grex_reactive_inflator_inflate:
 *
 * Performs a new inflation of this inflator's fragment into its target widget,
 * tracking new dependencies in the process.
 */
void
grex_reactive_inflator_inflate(GrexReactiveInflator *inflator) {
  GrexExpressionContext *context =
      grex_inflator_get_context(inflator->base_inflator);
  grex_expression_context_reset_dependencies(context);

  grex_inflator_inflate_existing_widget(inflator->base_inflator,
                                        inflator->widget, inflator->fragment,
                                        GREX_INFLATION_TRACK_DEPENDENCIES);
}
