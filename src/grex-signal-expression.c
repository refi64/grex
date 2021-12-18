/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "gpropz.h"
#include "grex-expression-context-private.h"
#include "grex-expression-private.h"
#include "grex-expression.h"
#include "grex-value-parser.h"

G_DECLARE_FINAL_TYPE(GrexSignalExpression, grex_signal_expression, GREX,
                     SIGNAL_EXPRESSION, GrexExpression)

struct _GrexSignalExpression {
  GrexExpression parent_instance;

  GrexExpression *object;
  char *signal;
  char *detail;

  GPtrArray *args;
};

enum {
  PROP_OBJECT = 1,
  PROP_SIGNAL,
  PROP_DETAIL,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexSignalExpression, grex_signal_expression,
              GREX_TYPE_EXPRESSION)

static void
grex_signal_expression_constructed(GObject *object) {
  GrexSignalExpression *signal_expr = GREX_SIGNAL_EXPRESSION(object);

  g_warn_if_fail(signal_expr->signal != NULL);
}

static void
grex_signal_expression_dispose(GObject *object) {
  GrexSignalExpression *signal_expr = GREX_SIGNAL_EXPRESSION(object);

  g_clear_object(&signal_expr->object);  // NOLINT
}

static void
grex_signal_expression_finalize(GObject *object) {
  GrexSignalExpression *signal_expr = GREX_SIGNAL_EXPRESSION(object);

  g_clear_pointer(&signal_expr->signal, g_free);  // NOLINT
}

static GPtrArray *
grex_signal_expression_evaluate_args(GrexSignalExpression *signal_expr,
                                     GrexExpressionContext *context,
                                     GrexExpressionEvaluationFlags flags,
                                     GrexValueParser *parser,
                                     const GSignalQuery *signal,
                                     GError **error) {
  if (signal_expr->args->len != signal->n_params) {
    grex_set_expression_evaluation_error(
        error, GREX_EXPRESSION(signal_expr),
        GREX_EXPRESSION_EVALUATION_ERROR_INVALID_ARGUMENT_COUNT,
        "Invalid number of arguments to '%s': expected %u, got %u",
        signal_expr->signal, signal->n_params, signal_expr->args->len);
    return NULL;
  }

  g_autoptr(GPtrArray) args =
      g_ptr_array_new_with_free_func((GDestroyNotify)grex_value_holder_unref);
  for (guint i = 0; i < signal_expr->args->len; i++) {
    GrexExpression *arg_expr = g_ptr_array_index(signal_expr->args, i);
    g_autoptr(GrexValueHolder) arg_value = grex_expression_evaluate(
        arg_expr, context, GREX_EXPRESSION_EVALUATION_PROPAGATE_FLAGS(flags),
        error);
    if (arg_value == NULL) {
      return NULL;
    }

    g_autoptr(GError) transform_error = NULL;
    g_autoptr(GrexValueHolder) transformed_arg =
        grex_value_parser_try_transform(
            parser, arg_value, signal->param_types[i], &transform_error);
    if (transformed_arg == NULL) {
      grex_set_expression_evaluation_error(
          error, GREX_EXPRESSION(arg_expr),
          GREX_EXPRESSION_EVALUATION_ERROR_INVALID_TYPE,
          "Failed to convert type for '%s' argument %u: %s",
          signal_expr->signal, i + 1, transform_error->message);
      return NULL;
    }

    g_ptr_array_add(args, g_steal_pointer(&transformed_arg));
  }

  return g_steal_pointer(&args);
}

static GrexValueHolder *
grex_signal_expression_evaluate(GrexExpression *expression,
                                GrexExpressionContext *context,
                                GrexExpressionEvaluationFlags flags,
                                GError **error) {
  GrexSignalExpression *signal_expr = GREX_SIGNAL_EXPRESSION(expression);

  g_autoptr(GArray) values = g_array_new(FALSE, TRUE, sizeof(GValue));
  g_array_set_clear_func(values, (GDestroyNotify)g_value_unset);
  g_array_set_size(values, 1);  // will grow out more later!

  GValue *target_value = &g_array_index(values, GValue, 0);
  GObject *target_object = NULL;

  if (signal_expr->object != NULL) {
    g_autoptr(GrexValueHolder) target_holder = grex_expression_evaluate(
        signal_expr->object, context,
        GREX_EXPRESSION_EVALUATION_PROPAGATE_FLAGS(flags), error);
    if (target_holder == NULL) {
      return NULL;
    }

    const GValue *value = grex_value_holder_get_value(target_holder);
    GType type = G_VALUE_TYPE(value);
    if (!g_type_is_a(type, G_TYPE_OBJECT)) {
      grex_set_expression_evaluation_error(
          error, signal_expr->object,
          GREX_EXPRESSION_EVALUATION_ERROR_INVALID_TYPE,
          "Cannot emit signal on type '%s'", g_type_name(type));
      return NULL;
    }

    g_value_init(target_value, type);
    g_value_copy(value, target_value);
    target_object = g_value_get_object(target_value);
  } else {
    // TODO: this won't work when derived contexts become a thing
    target_object = grex_expression_context_get_scope(context);
    g_value_init(target_value,
                 G_OBJECT_CLASS_TYPE(G_OBJECT_GET_CLASS(target_object)));
    g_value_set_object(target_value, target_object);
  }

  GType type = G_OBJECT_CLASS_TYPE(G_OBJECT_GET_CLASS(target_object));
  guint signal_id = g_signal_lookup(signal_expr->signal, type);
  if (signal_id == 0) {
    grex_set_expression_evaluation_error(
        error, expression, GREX_EXPRESSION_EVALUATION_ERROR_UNDEFINED_SIGNAL,
        "Undefined signal '%s'", signal_expr->signal);
    return NULL;
  }

  GSignalQuery query = {0};
  g_signal_query(signal_id, &query);
  g_warn_if_fail(query.signal_id != 0);

  GQuark detail = 0;
  if (signal_expr->detail != NULL) {
    if (!(query.signal_flags & G_SIGNAL_DETAILED)) {
      grex_set_expression_evaluation_error(
          error, expression, GREX_EXPRESSION_EVALUATION_ERROR_INVALID_DETAIL,
          "Signal '%s' does not take any detail", signal_expr->signal);
      return NULL;
    }

    // XXX: Not sure if we should be using g_quark_try_string instead, is it
    // invalid to emit a detail value that's not already a quark?
    detail = g_quark_from_string(signal_expr->detail);
  } else if (query.signal_flags & G_SIGNAL_DETAILED) {
    grex_set_expression_evaluation_error(
        error, expression, GREX_EXPRESSION_EVALUATION_ERROR_INVALID_DETAIL,
        "Signal '%s' needs a detail value", signal_expr->signal);
    return NULL;
  }

  GrexValueParser *parser = grex_value_parser_default();
  g_autoptr(GPtrArray) args = grex_signal_expression_evaluate_args(
      signal_expr, context, flags, parser, &query, error);
  if (args == NULL) {
    return NULL;
  }

  g_array_set_size(values, values->len + args->len);
  for (gsize i = 0; i < args->len; i++) {
    const GValue *source =
        grex_value_holder_get_value(g_ptr_array_index(args, i));
    GValue *dest = &g_array_index(values, GValue, i + 1);

    g_value_init(dest, G_VALUE_TYPE(source));
    g_value_copy(source, dest);
  }

  g_auto(GValue) result = G_VALUE_INIT;

  if (query.return_type != G_TYPE_NONE) {
    g_value_init(&result, query.return_type);
    g_signal_emitv((const GValue *)values->data, query.signal_id, detail,
                   &result);
  } else {
    g_signal_emitv((const GValue *)values->data, query.signal_id, detail, NULL);

    // XXX: Not sure how to return null values other than this.
    g_value_init(&result, G_TYPE_OBJECT);
    g_value_set_object(&result, NULL);
  }

  return grex_value_holder_new(&result);
}

static void
grex_signal_expression_class_init(GrexSignalExpressionClass *klass) {
  GrexExpressionClass *expr_class = GREX_EXPRESSION_CLASS(klass);
  expr_class->evaluate = grex_signal_expression_evaluate;

  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->constructed = grex_signal_expression_constructed;
  object_class->dispose = grex_signal_expression_dispose;
  object_class->finalize = grex_signal_expression_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_OBJECT] = g_param_spec_object(
      "object", "Object", "The object to enut the signal on.", G_TYPE_OBJECT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexSignalExpression, object,
                          PROP_OBJECT, properties[PROP_OBJECT], NULL);

  properties[PROP_SIGNAL] =
      g_param_spec_string("signal", "Signal", "The signal to emit.", NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexSignalExpression, signal,
                          PROP_SIGNAL, properties[PROP_SIGNAL], NULL);

  properties[PROP_DETAIL] =
      g_param_spec_string("detail", "Detail", "The signal's detail to emit.",
                          NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexSignalExpression, detail,
                          PROP_DETAIL, properties[PROP_DETAIL], NULL);
}

static void
grex_signal_expression_init(GrexSignalExpression *signal_expr) {
  signal_expr->args = g_ptr_array_new_with_free_func(g_object_unref);
}

/**
 * grex_signal_expression_new:
 * @location: (transfer none): This expression's source location.
 * @object: (transfer none) (nullable): The object to emit the signal on, or
 *          %NULL to emit it on context's scope.
 * @signal: The signal to emit.
 * @detail: (nullable): The detail for the signal.
 * @args: (array length=n_args) (transfer none): The arguments to pass to the
 *        signal.
 *
 * Creates an expression that emits the given signal and then evaluates to a
 * NULL object.
 *
 * Returns: (transfer none): The new expression.
 */
GrexExpression *
grex_signal_expression_new(GrexSourceLocation *location, GrexExpression *object,
                           const char *signal, const char *detail,
                           GrexExpression **args, gsize n_args) {
  // Technically it's bad practice to ever init stuff in the new function like
  // this, with going through the property mechanism preferred...but really, no
  // one can create individual expression instances without going through here
  // first anyway.
  GrexSignalExpression *signal_expr =
      g_object_new(grex_signal_expression_get_type(), "location", location,
                   "object", object, "signal", signal, "detail", detail, NULL);
  for (gsize i = 0; i < n_args; i++) {
    g_ptr_array_add(signal_expr->args, g_object_ref(args[i]));
  }

  return GREX_EXPRESSION(signal_expr);
}
