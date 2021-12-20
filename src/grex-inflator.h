/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment-host.h"
#include "grex-fragment.h"
#include "grex-property-directive.h"

G_BEGIN_DECLS

typedef enum {
  GREX_INFLATOR_DIRECTIVE_NONE = 0,
  GREX_INFLATOR_DIRECTIVE_NO_AUTO_ATTACH = 1 << 0,
} GrexInflatorDirectiveFlags;

typedef enum _GrexInflationFlags {
  GREX_INFLATION_NONE = 0,
  GREX_INFLATION_TRACK_DEPENDENCIES = 1 << 0,
} GrexInflationFlags;

typedef enum _GrexChildInflationFlags {
  GREX_CHILD_INFLATION_NONE = 0,
  GREX_CHILD_INFLATION_IGNORE_STRUCTURAL_DIRECTIVES = 1 << 0,
} GrexChildInflationFlags;

#define GREX_TYPE_INFLATOR grex_inflator_get_type()
G_DECLARE_FINAL_TYPE(GrexInflator, grex_inflator, GREX, INFLATOR, GObject)

GrexInflator *grex_inflator_new(GrexExpressionContext *context);
GrexInflator *grex_inflator_new_with_scope(GObject *scope);

GrexExpressionContext *grex_inflator_get_context(GrexInflator *inflator);

void grex_inflator_take_directives(GrexInflator *inflator,
                                   GrexInflatorDirectiveFlags flags,
                                   ...) G_GNUC_NULL_TERMINATED;
void grex_inflator_add_directives(GrexInflator *inflator,
                                  GrexInflatorDirectiveFlags flags,
                                  ...) G_GNUC_NULL_TERMINATED;
void grex_inflator_add_directivesv(GrexInflator *inflator,
                                   GrexInflatorDirectiveFlags flags,
                                   guint n_directives,
                                   GrexDirectiveFactory **directives);

GObject *grex_inflator_inflate_new_target(GrexInflator *inflator,
                                          GrexFragment *fragment,
                                          GrexInflationFlags flags);
void grex_inflator_inflate_existing_target(GrexInflator *inflator,
                                           GObject *target,
                                           GrexFragment *fragment,
                                           GrexInflationFlags flags);

void grex_inflator_inflate_child(GrexInflator *inflator,
                                 GrexFragmentHost *parent, GrexKey *key,
                                 GrexFragment *child, GrexInflationFlags flags,
                                 GrexChildInflationFlags child_flags);

G_END_DECLS
