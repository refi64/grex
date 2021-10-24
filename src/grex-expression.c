/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-expression.h"

#include "gpropz.h"
#include "grex-enums.h"
#include "grex-expression-private.h"

typedef struct {
  GrexSourceLocation *location;
  gboolean is_constant;
} GrexExpressionPrivate;

enum {
  PROP_LOCATION = 1,
  PROP_IS_CONSTANT,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_QUARK("grex-expression-parse-error-quark", grex_expression_parse_error)
G_DEFINE_QUARK("grex-expression-evaluation-error-quark",
               grex_expression_evaluation_error)

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(GrexExpression, grex_expression,
                                    G_TYPE_OBJECT)

G_DECLARE_FINAL_TYPE(GrexExpressionImpl, grex_expression_impl, GREX,
                     EXPRESSION_IMPL, GrexExpression)

static void
grex_expression_dispose(GObject *object) {
  GrexExpression *expression = GREX_EXPRESSION(object);
  GrexExpressionPrivate *priv =
      grex_expression_get_instance_private(expression);
  g_clear_object(&priv->location);  // NOLINT
}

static void
grex_expression_class_init(GrexExpressionClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_expression_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_LOCATION] = g_param_spec_object(
      "location", "Location",
      "Location in the expression string of this expression.",
      GREX_TYPE_SOURCE_LOCATION, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property_private(object_class, GrexExpression, location,
                                  PROP_LOCATION, properties[PROP_LOCATION],
                                  NULL);

  properties[PROP_IS_CONSTANT] =
      g_param_spec_boolean("is-constant", "Is a constant",
                           "Whether or not this is a constant expression.",
                           FALSE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property_private(object_class, GrexExpression, is_constant,
                                  PROP_IS_CONSTANT,
                                  properties[PROP_IS_CONSTANT], NULL);
}

static void
grex_expression_init(GrexExpression *expression) {}

/**
 * grex_expression_parse:
 *
 * TODO
 *
 * Returns: (transfer full): TODO
 */
GrexExpression *
grex_expression_parse(const char *string, gssize len,
                      GrexSourceLocation *location, GError **error) {
  // TODO: actual parsing
  if (g_ascii_isdigit(*string)) {
    g_auto(GValue) value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_LONG);
    g_value_set_long(&value, strtol(string, NULL, 10));
    return grex_constant_value_expression_new(location, &value);
  } else {
    return grex_property_expression_new(
        location, NULL, len != -1 ? g_strndup(string, len) : g_strdup(string));
  }
}

/**
 * grex_expression_get_location:
 *
 * Gets the source location for this expression.
 *
 * Returns: (transfer none): The source location.
 */
GPROPZ_DEFINE_RO(GrexSourceLocation *, GrexExpression, grex_expression,
                 location, properties[PROP_LOCATION])

/**
 * grex_expression_is_constant:
 *
 * Checks if this is a constant expression.
 *
 * Returns: TRUE if this expression is constant.
 */
gboolean
grex_expression_is_constant(GrexExpression *expression) {
  GrexExpressionPrivate *priv =
      grex_expression_get_instance_private(expression);
  return priv->is_constant;
}

/**
 * grex_expression_evaluate:
 * @context: (transfer none): The context to evaluate this expression in.
 * @flags: Flags for this expression's evaluation.
 * @error: Return location for a #GError.
 *
 * Evaluates this expression using the scope from the given context.
 *
 * Returns: (transfer full): The evaluation result, or NULL on error.
 */
GrexValueHolder *
grex_expression_evaluate(GrexExpression *expression,
                         GrexExpressionContext *context,
                         GrexExpressionEvaluationFlags flags, GError **error) {
  GrexExpressionClass *expression_class = GREX_EXPRESSION_GET_CLASS(expression);
  g_return_val_if_fail(expression_class->evaluate != NULL, NULL);
  return expression_class->evaluate(expression, context, flags, error);
}

void
grex_set_expression_evaluation_error(GError **error, GrexExpression *expression,
                                     int code, const char *format, ...) {
  va_list va;
  va_start(va, format);
  grex_set_located_error_va(error, grex_expression_get_location(expression),
                            GREX_EXPRESSION_EVALUATION_ERROR, code, format, va);
  va_end(va);
}
