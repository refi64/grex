/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-expression-context.h"

#ifndef _GREX_INTERNAL
#error "This is internal stuff, you shouldn't be here!"
#endif

GObject *grex_expression_context_find_object_with_property(
    GrexExpressionContext *context, const char *property);

void grex_expression_context_emit_changed(GrexExpressionContext *context);
