/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-directive.h"

// TODO: figure out if there's a better way of adding class-level private data
struct _GrexDirectiveClassPrivate {
  char *name;
  GType auto_attach_type;
};

G_DEFINE_ABSTRACT_TYPE(GrexDirective, grex_directive, G_TYPE_OBJECT)

static void
grex_directive_class_init(GrexDirectiveClass *klass) {
  klass->priv = g_new0(GrexDirectiveClassPrivate, 1);
}

static void
grex_directive_init(GrexDirective *directive) {}

/**
 * grex_directive_class_get_name:
 * @klass: The directive class to get the name of.
 *
 * Returns the name assigned to the given directive class.
 *
 * Returns: The directive's name, or NULL if none was set.
 */
const char *
grex_directive_class_get_name(GrexDirectiveClass *klass) {
  return klass->priv->name;
}

/**
 * grex_directive_class_get_auto_attach:
 * @klass: The directive class to get the auto attach type for.
 *
 * Returns the type this directive with auto-attach to.
 *
 * Returns: The #GType, or 0 if none was set.
 */
GType
grex_directive_class_get_auto_attach(GrexDirectiveClass *klass) {
  return klass->priv->auto_attach_type;
}

/**
 * grex_directive_class_set_name:
 * @klass: The directive class.
 * @name: The new name.
 *
 * Assigns the given name to the directive class.
 */
void
grex_directive_class_set_name(GrexDirectiveClass *klass, const char *name) {
  g_clear_pointer(&klass->priv->name, g_free);
  klass->priv->name = g_strdup(name);
}

/**
 * grex_directive_class_set_auto_attach:
 * @klass: The directive class.
 * @name: The new auto-attach type, or 0 to disable auto-attach support.
 *
 * Assigns the type this directive will attempt to auto-attach to.
 */
void
grex_directive_class_set_auto_attach(GrexDirectiveClass *klass,
                                     GType auto_attach_type) {
  klass->priv->auto_attach_type = auto_attach_type;
}
