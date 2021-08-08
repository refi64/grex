/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-source-location.h"
#include "grex-value-holder.h"

G_BEGIN_DECLS

typedef enum {
  GREX_BINDING_TYPE_CONSTANT,
  GREX_BINDING_TYPE_EXPRESSION_1WAY,
  GREX_BINDING_TYPE_EXPRESSION_2WAY,
  GREX_BINDING_TYPE_COMPOUND,
} GrexBindingType;

static G_GNUC_UNUSED gboolean
grex_binding_type_is_expression(GrexBindingType type) {
  return type == GREX_BINDING_TYPE_EXPRESSION_1WAY ||
         type == GREX_BINDING_TYPE_EXPRESSION_2WAY;
}

#define GREX_TYPE_BINDING grex_binding_get_type()
G_DECLARE_FINAL_TYPE(GrexBinding, grex_binding, GREX, BINDING, GObject)

GrexBindingType grex_binding_get_binding_type(GrexBinding *binding);
GrexSourceLocation *grex_binding_get_location(GrexBinding *binding);

GrexValueHolder *grex_binding_evaluate(GrexBinding *binding, GError **error);

#define GREX_TYPE_BINDING_BUILDER grex_binding_builder_get_type()
G_DECLARE_FINAL_TYPE(GrexBindingBuilder, grex_binding_builder, GREX,
                     BINDING_BUILDER, GObject)

GrexBindingBuilder *grex_binding_builder_new();

void grex_binding_builder_add_constant(GrexBindingBuilder *builder,
                                       const char *content);

GrexBinding *grex_binding_builder_build(GrexBindingBuilder *builder,
                                        GrexSourceLocation *location);

G_END_DECLS
