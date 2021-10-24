/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-property-directive.h"

G_DEFINE_ABSTRACT_TYPE(GrexPropertyDirective, grex_property_directive,
                       GREX_TYPE_DIRECTIVE)

static void
grex_property_directive_default(GrexPropertyDirective *directive,
                                 GrexFragmentHost *attach) {}

static void
grex_property_directive_class_init(GrexPropertyDirectiveClass *klass) {
  klass->attach = grex_property_directive_default;
  klass->update = grex_property_directive_default;
  klass->detach = grex_property_directive_default;
}

static void
grex_property_directive_init(GrexPropertyDirective *directive) {}
