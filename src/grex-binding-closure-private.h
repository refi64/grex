/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-binding.h"
#include "grex-config.h"
#include "grex-expression-context.h"

#ifndef _GREX_INTERNAL
#error "This is internal stuff, you shouldn't be here!"
#endif

typedef struct _GrexBindingClosure GrexBindingClosure;

GClosure *grex_binding_closure_create(GrexBinding *binding,
                                      GrexExpressionContext *context);
