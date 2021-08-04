/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-source-location.h"

G_BEGIN_DECLS

#define GREX_TYPE_BINDING grex_binding_get_type()
G_DECLARE_FINAL_TYPE(GrexBinding, grex_binding, GREX, BINDING, GObject)

GrexBinding *grex_binding_new(const char *target, GrexSourceLocation *location);

const char *grex_binding_get_target(GrexBinding *binding);
GrexSourceLocation *grex_binding_get_location(GrexBinding *binding);

void grex_binding_add_constant(GrexBinding *binding, const char *content);

char *grex_binding_evaluate(GrexBinding *binding, GError **error);

G_END_DECLS
