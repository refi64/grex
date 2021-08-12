/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-expression.h"

#ifndef _GREX_INTERNAL
#error "This is internal stuff, you shouldn't be here!"
#endif

#define GREX_EXPRESSION_EVALUATION_PROPAGATE_FLAGS(flags) \
  ((flags) & (GREX_EXPRESSION_EVALUATION_TRACK_DEPENDENCIES))

struct _GrexExpressionClass {
  GObjectClass parent_class;

  GrexValueHolder *(*evaluate)(GrexExpression *expression,
                               GrexExpressionContext *context,
                               GrexExpressionEvaluationFlags flags,
                               GError **error);
};

void grex_set_expression_evaluation_error(GError **error,
                                          GrexExpression *expression, int code,
                                          const char *format, ...)
    G_GNUC_PRINTF(4, 5);
