/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-property-directive.h"

G_DEFINE_ABSTRACT_TYPE(GrexPropertyDirective, grex_property_directive,
                       GREX_TYPE_DIRECTIVE)
G_DEFINE_ABSTRACT_TYPE(GrexPropertyDirectiveFactory,
                       grex_property_directive_factory,
                       GREX_TYPE_DIRECTIVE_FACTORY)

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

static gboolean
grex_property_directive_factory_should_auto_attach_default(
    GrexPropertyDirectiveFactory *factory, GrexFragmentHost *host,
    GrexFragment *fragment) {
  return FALSE;
}

static void
grex_property_directive_factory_class_init(
    GrexPropertyDirectiveFactoryClass *klass) {
  klass->should_auto_attach =
      grex_property_directive_factory_should_auto_attach_default;
}

static void
grex_property_directive_factory_init(GrexPropertyDirectiveFactory *factory) {}

/**
 * grex_property_directive_factory_create: (virtual create)
 *
 * Creates and returns a new #GrexPropertyDirective.
 *
 * Returns: (transfer full): The new directive.
 */
GrexPropertyDirective *
grex_property_directive_factory_create(GrexPropertyDirectiveFactory *factory) {
  GrexPropertyDirectiveFactoryClass *klass =
      GREX_PROPERTY_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->create != NULL, NULL);

  return klass->create(factory);
}

/**
 * grex_property_directive_factory_should_auto_attach:
 *    (virtual should_auto_attach)
 *
 * Determines if this directive should be auto-attached to the given fragment
 * and host.
 *
 * Returns: %TRUE if this factory's directive should be auto-attached.
 */
gboolean
grex_property_directive_factory_should_auto_attach(
    GrexPropertyDirectiveFactory *factory, GrexFragmentHost *host,
    GrexFragment *fragment) {
  GrexPropertyDirectiveFactoryClass *klass =
      GREX_PROPERTY_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->should_auto_attach != NULL, FALSE);

  return klass->should_auto_attach(factory, host, fragment);
}
