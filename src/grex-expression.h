/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-expression-context.h"
#include "grex-source-location.h"
#include "grex-value-holder.h"

G_BEGIN_DECLS

#define GREX_EXPRESSION_PARSE_ERROR grex_expression_parse_error_quark()
GQuark grex_expression_parse_error_quark();

typedef enum {
  GREX_EXPRESSION_EVALUATION_ERROR_UNDEFINED_NAME,
  GREX_EXPRESSION_EVALUATION_ERROR_UNDEFINED_PROPERTY,
  GREX_EXPRESSION_EVALUATION_ERROR_UNDEFINED_SIGNAL,
  GREX_EXPRESSION_EVALUATION_ERROR_INVALID_TYPE,
  GREX_EXPRESSION_EVALUATION_ERROR_INVALID_ARGUMENT_COUNT,
  GREX_EXPRESSION_EVALUATION_ERROR_INVALID_DETAIL,
} GrexExpressionEvaluationError;

#define GREX_EXPRESSION_EVALUATION_ERROR \
  grex_expression_evaluation_error_quark()
GQuark grex_expression_evaluation_error_quark();

#define GREX_TYPE_EXPRESSION grex_expression_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexExpression, grex_expression, GREX, EXPRESSION,
                         GObject)

typedef enum {
  GREX_EXPRESSION_EVALUATION_NONE = 0,
  GREX_EXPRESSION_EVALUATION_ENABLE_PUSH = 1 << 0,
  GREX_EXPRESSION_EVALUATION_TRACK_DEPENDENCIES = 1 << 1,
} GrexExpressionEvaluationFlags;

GrexExpression *grex_expression_parse(const char *string, gssize len,
                                      GrexSourceLocation *location,
                                      GError **error);

GrexSourceLocation *grex_expression_get_location(GrexExpression *expression);
gboolean grex_expression_is_constant(GrexExpression *expression);

GrexValueHolder *grex_expression_evaluate(GrexExpression *expression,
                                          GrexExpressionContext *context,
                                          GrexExpressionEvaluationFlags flags,
                                          GError **error);

GrexExpression *grex_constant_value_expression_new(GrexSourceLocation *location,
                                                   const GValue *value);

GrexExpression *grex_property_expression_new(GrexSourceLocation *location,
                                             GrexExpression *object,
                                             const char *name);

GrexExpression *grex_signal_expression_new(GrexSourceLocation *location,
                                           GrexExpression *object,
                                           const char *signal,
                                           const char *detail,
                                           GrexExpression **args, gsize n_args);

G_END_DECLS
