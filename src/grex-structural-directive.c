/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-structural-directive.h"

#include "grex-inflator.h"

G_DEFINE_ABSTRACT_TYPE(GrexStructuralDirective, grex_structural_directive,
                       GREX_TYPE_DIRECTIVE)
G_DEFINE_ABSTRACT_TYPE(GrexStructuralDirectiveFactory,
                       grex_structural_directive_factory,
                       GREX_TYPE_DIRECTIVE_FACTORY)

static void
grex_structural_directive_apply_default(GrexStructuralDirective *directive,
                                        GrexInflator *inflator,
                                        GrexFragmentHost *parent, GrexKey *key,
                                        GrexFragment *child,
                                        GrexInflationFlags flags,
                                        GrexChildInflationFlags child_flags) {}

static void
grex_structural_directive_class_init(GrexStructuralDirectiveClass *klass) {
  klass->apply = grex_structural_directive_apply_default;
}

static void
grex_structural_directive_init(GrexStructuralDirective *directive) {}

static void
grex_structural_directive_factory_class_init(
    GrexStructuralDirectiveFactoryClass *klass) {}

static void
grex_structural_directive_factory_init(
    GrexStructuralDirectiveFactory *factory) {}

/**
 * grex_structural_directive_factory_create: (virtual create)
 *
 * Creates and returns a new #GrexStructuralDirective.
 *
 * Returns: (transfer full): The new directive.
 */
GrexStructuralDirective *
grex_structural_directive_factory_create(
    GrexStructuralDirectiveFactory *factory) {
  GrexStructuralDirectiveFactoryClass *klass =
      GREX_STRUCTURAL_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->create != NULL, NULL);

  return klass->create(factory);
}
