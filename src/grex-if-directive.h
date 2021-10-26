/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-structural-directive.h"

G_BEGIN_DECLS

#define GREX_TYPE_IF_DIRECTIVE grex_if_directive_get_type()
G_DECLARE_FINAL_TYPE(GrexIfDirective, grex_if_directive, GREX, IF_DIRECTIVE,
                     GrexStructuralDirective)

#define GREX_TYPE_IF_DIRECTIVE_FACTORY grex_if_directive_factory_get_type()
G_DECLARE_FINAL_TYPE(GrexIfDirectiveFactory, grex_if_directive_factory, GREX,
                     IF_DIRECTIVE_FACTORY, GrexStructuralDirectiveFactory)

GrexIfDirectiveFactory *grex_if_directive_factory_new();

G_END_DECLS
