/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "gpropz.h"
#include "grex-expression-context-private.h"
#include "grex-expression-private.h"
#include "grex-expression.h"

G_DECLARE_FINAL_TYPE(GrexPropertyExpression, grex_property_expression, GREX,
                     PROPERTY_EXPRESSION, GrexExpression)

struct _GrexPropertyExpression {
  GrexExpression parent_instance;

  GrexExpression *object;
  char *name;
};

enum {
  PROP_OBJECT = 1,
  PROP_NAME,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexPropertyExpression, grex_property_expression,
              GREX_TYPE_EXPRESSION)

static void
grex_property_expression_dispose(GObject *object) {
  GrexPropertyExpression *expression = GREX_PROPERTY_EXPRESSION(object);

  g_clear_object(&expression->object);  // NOLINT
}

static void
grex_property_expression_finalize(GObject *object) {
  GrexPropertyExpression *expression = GREX_PROPERTY_EXPRESSION(object);

  g_clear_pointer(&expression->name, g_free);
}

static void
on_notify_property_changed(GObject *object, GParamSpec *pspec,
                           gpointer user_data) {
  GrexExpressionContext *context = GREX_EXPRESSION_CONTEXT(user_data);
  grex_expression_context_emit_changed(context);
}

typedef struct {
  GObject *object;
  char *property;
} PushValueData;

static void
push_value_data_free(gpointer user_data) {
  PushValueData *data = user_data;
  g_object_unref(data->object);
  g_free(data->property);
}

static void
on_push_value(const GValue *value, gpointer user_data) {
  PushValueData *data = user_data;
  g_object_set_property(data->object, data->property, value);
}

static GrexValueHolder *
grex_property_expression_evaluate(GrexExpression *expression,
                                  GrexExpressionContext *context,
                                  GrexExpressionEvaluationFlags flags,
                                  GError **error) {
  GrexPropertyExpression *property_expression =
      GREX_PROPERTY_EXPRESSION(expression);

  // Hoisted out here to ensure it always lives longer than the object within,
  // without needing any refcount hackery.
  g_autoptr(GrexValueHolder) lookup_target_holder = NULL;
  GObject *lookup_target = NULL;
  if (property_expression->object != NULL) {
    lookup_target_holder = grex_expression_evaluate(
        property_expression->object, context,
        GREX_EXPRESSION_EVALUATION_PROPAGATE_FLAGS(flags), error);
    if (lookup_target_holder == NULL) {
      return NULL;
    }

    const GValue *value = grex_value_holder_get_value(lookup_target_holder);
    GType type = G_VALUE_TYPE(value);
    if (!g_type_is_a(type, G_TYPE_OBJECT)) {
      grex_set_expression_evaluation_error(
          error, property_expression->object,
          GREX_EXPRESSION_EVALUATION_ERROR_INVALID_TYPE,
          "Cannot get property on type '%s'", g_type_name(type));
      return NULL;
    }

    lookup_target = g_value_get_object(value);

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(lookup_target),
                                     property_expression->name) == NULL) {
      grex_set_expression_evaluation_error(
          error, expression,
          GREX_EXPRESSION_EVALUATION_ERROR_UNDEFINED_PROPERTY,
          "Undefined property '%s'", property_expression->name);
      return NULL;
    }
  } else {
    lookup_target = grex_expression_context_find_object_with_property(
        context, property_expression->name);
    if (lookup_target == NULL) {
      grex_set_expression_evaluation_error(
          error, expression, GREX_EXPRESSION_EVALUATION_ERROR_UNDEFINED_NAME,
          "Undefined name '%s'", property_expression->name);
      return NULL;
    }
  }

  g_warn_if_fail(lookup_target != NULL);

  g_auto(GValue) value = G_VALUE_INIT;
  g_object_get_property(lookup_target, property_expression->name, &value);

  if (flags & GREX_EXPRESSION_EVALUATION_TRACK_DEPENDENCIES) {
    g_autofree char *signal =
        g_strdup_printf("notify::%s", property_expression->name);
    g_signal_connect_object(lookup_target, signal,
                            G_CALLBACK(on_notify_property_changed), context, 0);
  }

  if (flags & GREX_EXPRESSION_EVALUATION_ENABLE_PUSH) {
    PushValueData *data = g_new0(PushValueData, 1);
    data->object = g_object_ref(lookup_target);
    data->property = g_strdup(property_expression->name);
    return grex_value_holder_new_with_push_handler(&value, on_push_value, data,
                                                   push_value_data_free);
  } else {
    return grex_value_holder_new(&value);
  }
}

static void
grex_property_expression_class_init(GrexPropertyExpressionClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_property_expression_dispose;
  object_class->finalize = grex_property_expression_finalize;

  gpropz_class_init_property_functions(object_class);

  GrexExpressionClass *expression_class = GREX_EXPRESSION_CLASS(klass);

  expression_class->evaluate = grex_property_expression_evaluate;

  properties[PROP_OBJECT] = g_param_spec_object(
      "object", "Object",
      "The object to retrieve the property from. NULL to get from the scopes.",
      G_TYPE_OBJECT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexPropertyExpression, object,
                          PROP_OBJECT, properties[PROP_OBJECT], NULL);

  properties[PROP_NAME] =
      g_param_spec_string("name", "Name", "The property name.", "",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexPropertyExpression, name, PROP_NAME,
                          properties[PROP_NAME], NULL);
}

static void
grex_property_expression_init(GrexPropertyExpression *expression) {}

/**
 * grex_property_expression_new:
 * @location: (transfer none): This expression's source location.
 * @object: (transfer none) (allow-none): The expression representing the object
 *          to get this property from, or NULL to get it from the context's
 *          scopes.
 * @name: The property name.
 *
 * Creates an expression that retrieves a property from an object (or the global
 * scopes if the object is NULL).
 *
 * Returns: (transfer none): The new expression.
 */
GrexExpression *
grex_property_expression_new(GrexSourceLocation *location,
                             GrexExpression *object, const char *name) {
  return g_object_new(grex_property_expression_get_type(), "location", location,
                      "object", object, "name", name, NULL);
}
