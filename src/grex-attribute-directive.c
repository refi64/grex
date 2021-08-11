/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-attribute-directive.h"

G_DEFINE_ABSTRACT_TYPE(GrexAttributeDirective, grex_attribute_directive,
                       GREX_TYPE_DIRECTIVE)

static void
grex_attribute_directive_default(GrexAttributeDirective *directive,
                                 GrexFragmentHost *attach) {}

static void
grex_attribute_directive_class_init(GrexAttributeDirectiveClass *klass) {
  klass->attach = grex_attribute_directive_default;
  klass->update = grex_attribute_directive_default;
  klass->detach = grex_attribute_directive_default;
}

static void
grex_attribute_directive_init(GrexAttributeDirective *directive) {}
