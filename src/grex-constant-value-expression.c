/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "gpropz.h"
#include "grex-expression-context-private.h"
#include "grex-expression-private.h"
#include "grex-expression.h"

G_DECLARE_FINAL_TYPE(GrexConstantValueExpression,
                     grex_constant_value_expression, GREX,
                     CONSTANT_VALUE_EXPRESSION, GrexExpression)

struct _GrexConstantValueExpression {
  GrexExpression parent_instance;

  GrexValueHolder *value;
};

enum {
  PROP_VALUE = 1,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexConstantValueExpression, grex_constant_value_expression,
              GREX_TYPE_EXPRESSION)

static void
grex_constant_value_expression_constructed(GObject *object) {
  GrexConstantValueExpression *const_expr =
      GREX_CONSTANT_VALUE_EXPRESSION(object);

  g_warn_if_fail(const_expr->value != NULL);
  g_warn_if_fail(grex_value_holder_can_push(const_expr->value));
}

static void
grex_constant_value_expression_finalize(GObject *object) {
  GrexConstantValueExpression *const_expr =
      GREX_CONSTANT_VALUE_EXPRESSION(object);

  g_clear_pointer(&const_expr->value, grex_value_holder_unref);  // NOLINT
}

static GrexValueHolder *
grex_constant_value_expression_evaluate(GrexExpression *expression,
                                        GrexExpressionContext *context,
                                        GrexExpressionEvaluationFlags flags,
                                        GError **error) {
  GrexConstantValueExpression *const_expr =
      GREX_CONSTANT_VALUE_EXPRESSION(expression);
  return grex_value_holder_ref(const_expr->value);
}

static void
grex_constant_value_expression_class_init(
    GrexConstantValueExpressionClass *klass) {
  GrexExpressionClass *expr_class = GREX_EXPRESSION_CLASS(klass);
  expr_class->evaluate = grex_constant_value_expression_evaluate;

  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->constructed = grex_constant_value_expression_constructed;
  object_class->finalize = grex_constant_value_expression_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_VALUE] = g_param_spec_boxed(
      "value", "Value.", "The constant value this contains.",
      GREX_TYPE_VALUE_HOLDER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  gpropz_install_property(object_class, GrexConstantValueExpression, value,
                          PROP_VALUE, properties[PROP_VALUE], NULL);
}

static void
grex_constant_value_expression_init(GrexConstantValueExpression *const_expr) {}

/**
 * grex_constant_value_expression_new:
 * @location: (transfer none): This expression's source location.
 * @value: The expression's value.
 *
 * Creates a constant expression that always evaluates to the given value.
 *
 * Returns: (transfer none): The new expression.
 */
GrexExpression *
grex_constant_value_expression_new(GrexSourceLocation *location,
                                   const GValue *value) {
  g_autoptr(GrexValueHolder) value_holder = grex_value_holder_new(value);
  return g_object_new(grex_constant_value_expression_get_type(), "location",
                      location, "is-constant", TRUE, "value", value_holder,
                      NULL);
}
