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

static gboolean
grex_directive_factory_should_auto_attach_default(GrexDirectiveFactory *factory,
                                                  GrexFragmentHost *host,
                                                  GrexFragment *fragment) {
  return FALSE;
}

static void
grex_directive_factory_class_init(GrexDirectiveFactoryClass *klass) {
  klass->should_auto_attach = grex_directive_factory_should_auto_attach_default;
}

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
 * grex_directive_factory_create: (virtual create)
 *
 * Creates and returns a new #GrexDirective.
 *
 * Returns: (transfer full): The new directive.
 */
GrexDirective *
grex_directive_factory_create(GrexDirectiveFactory *factory) {
  GrexDirectiveFactoryClass *klass = GREX_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->create != NULL, NULL);

  return klass->create(factory);
}

/**
 * grex_directive_factory_should_auto_attach: (virtual should_auto_attach)
 *
 * Creates and returns a new #GrexDirective.
 *
 * Returns: %TRUE if this factory's directive should be auto-attached to the
 *          given fragment and host.
 */
gboolean
grex_directive_factory_should_auto_attach(GrexDirectiveFactory *factory,
                                          GrexFragmentHost *host,
                                          GrexFragment *fragment) {
  GrexDirectiveFactoryClass *klass = GREX_DIRECTIVE_FACTORY_GET_CLASS(factory);
  g_return_val_if_fail(klass->should_auto_attach != NULL, FALSE);

  return klass->should_auto_attach(factory, host, fragment);
}
