/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-directive.h"

G_DEFINE_ABSTRACT_TYPE(GrexDirective, grex_directive, G_TYPE_OBJECT)
G_DEFINE_ABSTRACT_TYPE(GrexDirectiveFactory, grex_directive_factory,
                       G_TYPE_OBJECT)

static void
grex_directive_class_init(GrexDirectiveClass *klass) {}

static void
grex_directive_init(GrexDirective *directive) {}

static void
grex_directive_factory_class_init(GrexDirectiveFactoryClass *klass) {}

static void
grex_directive_factory_init() {}

/**
 * grex_directive_factory_get_name: (virtual get_name)
 *
 * Gets the name of the directive this factory creates.
 *
 * Returns: The directive's name.
 */
const char *
grex_directive_factory_get_name(GrexDirectiveFactory *factory) {
  GrexDirectiveFactoryClass *klass = GREX_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->get_name != NULL, NULL);

  return klass->get_name(factory);
}

/**
 * grex_directive_factory_get_property_format: (virtual get_property_format)
 *
 * Gets the format of property assignments for this factory's directive.
 *
 * Returns: The directive's property format.
 */
GrexDirectivePropertyFormat
grex_directive_factory_get_property_format(GrexDirectiveFactory *factory) {
  GrexDirectiveFactoryClass *klass = GREX_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->get_property_format != NULL, 0);

  return klass->get_property_format(factory);
}
