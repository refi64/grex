/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-directive.h"
#include "grex-fragment-host.h"

G_BEGIN_DECLS

typedef struct _GrexInflator GrexInflator;
typedef enum _GrexInflationFlags GrexInflationFlags;
typedef enum _GrexChildInflationFlags GrexChildInflationFlags;

#define GREX_TYPE_STRUCTURAL_DIRECTIVE grex_structural_directive_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexStructuralDirective, grex_structural_directive,
                         GREX, STRUCTURAL_DIRECTIVE, GrexDirective)

struct _GrexStructuralDirectiveClass {
  GrexDirectiveClass parent_class;

  void (*apply)(GrexStructuralDirective *directive, GrexInflator *inflator,
                GrexFragmentHost *parent, GrexKey *key, GrexFragment *child,
                GrexInflationFlags flags, GrexChildInflationFlags child_flags);

  gpointer padding[15];
};

#define GREX_TYPE_STRUCTURAL_DIRECTIVE_FACTORY \
  grex_structural_directive_factory_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexStructuralDirectiveFactory,
                         grex_structural_directive_factory, GREX,
                         STRUCTURAL_DIRECTIVE_FACTORY, GrexDirectiveFactory)

struct _GrexStructuralDirectiveFactoryClass {
  GrexDirectiveFactoryClass parent_class;

  GrexStructuralDirective *(*create)(GrexStructuralDirectiveFactory *factory);

  gpointer padding[15];
};

GrexStructuralDirective *grex_structural_directive_factory_create(
    GrexStructuralDirectiveFactory *factory);

G_END_DECLS
