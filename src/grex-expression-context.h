/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-source-location.h"
#include "grex-value-holder.h"

G_BEGIN_DECLS

#define GREX_TYPE_EXPRESSION_CONTEXT grex_expression_context_get_type()
G_DECLARE_FINAL_TYPE(GrexExpressionContext, grex_expression_context, GREX,
                     EXPRESSION_CONTEXT, GObject)

GrexExpressionContext *grex_expression_context_new(GObject *scope);

GrexExpressionContext *
grex_expression_context_clone(GrexExpressionContext *base);

GObject *grex_expression_context_get_scope(GrexExpressionContext *context);

gboolean grex_expression_context_insert(GrexExpressionContext *context,
                                        const char *name, const GValue *value);
gboolean grex_expression_context_find_name(GrexExpressionContext *context,
                                           const char *name, GValue *dest,
                                           GObject **originating_scope);

void grex_expression_context_reset_dependencies(GrexExpressionContext *context);

G_END_DECLS
