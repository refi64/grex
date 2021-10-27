/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-if-directive.h"

#include "gpropz.h"
#include "grex-inflator.h"

struct _GrexIfDirective {
  GrexStructuralDirective parent_instance;

  gboolean value;
};

enum {
  PROP_VALUE = 1,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {0};

G_DEFINE_FINAL_TYPE(GrexIfDirective, grex_if_directive,
                    GREX_TYPE_STRUCTURAL_DIRECTIVE)

static void
grex_if_directive_apply(GrexStructuralDirective *directive,
                        GrexInflator *inflator, GrexFragmentHost *parent,
                        guintptr key, GrexFragment *child,
                        GrexInflationFlags flags,
                        GrexChildInflationFlags child_flags) {
  GrexIfDirective *if_directive = GREX_IF_DIRECTIVE(directive);
  if (if_directive->value) {
    grex_inflator_inflate_child(inflator, parent, key, child, flags,
                                child_flags);
  }
}

static void
grex_if_directive_class_init(GrexIfDirectiveClass *klass) {
  GrexStructuralDirectiveClass *directive_class =
      GREX_STRUCTURAL_DIRECTIVE_CLASS(klass);
  directive_class->apply = grex_if_directive_apply;

  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  gpropz_class_init_property_functions(object_class);

  properties[PROP_VALUE] = g_param_spec_boolean(
      "value", "Value.", "Determines if the annotated fragment is inflated.",
      FALSE, G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexIfDirective, value, PROP_VALUE,
                          properties[PROP_VALUE], NULL);
}

static void
grex_if_directive_init(GrexIfDirective *directive) {}

struct _GrexIfDirectiveFactory {
  GrexStructuralDirectiveFactory parent_instance;
};

G_DEFINE_FINAL_TYPE(GrexIfDirectiveFactory, grex_if_directive_factory,
                    GREX_TYPE_STRUCTURAL_DIRECTIVE_FACTORY)

static const char *
grex_if_directive_factory_get_name(GrexDirectiveFactory *factory) {
  return "Grex.if";
}

static GrexDirectivePropertyFormat
grex_if_directive_factory_get_property_format(GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_IMPLICIT_VALUE;
}

static GrexStructuralDirective *
grex_if_directive_factory_create(GrexStructuralDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_IF_DIRECTIVE, NULL);
}

static void
grex_if_directive_factory_class_init(GrexIfDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *directive_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);

  directive_class->get_name = grex_if_directive_factory_get_name;
  directive_class->get_property_format =
      grex_if_directive_factory_get_property_format;

  GrexStructuralDirectiveFactoryClass *struct_factory_class =
      GREX_STRUCTURAL_DIRECTIVE_FACTORY_CLASS(klass);

  struct_factory_class->create = grex_if_directive_factory_create;
}

static void
grex_if_directive_factory_init(GrexIfDirectiveFactory *factory) {}

GrexIfDirectiveFactory *
grex_if_directive_factory_new() {
  return g_object_new(GREX_TYPE_IF_DIRECTIVE_FACTORY, NULL);
}
