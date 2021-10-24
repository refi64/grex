/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-directive.h"
#include "grex-fragment-host.h"

#define GREX_TYPE_PROPERTY_DIRECTIVE grex_property_directive_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexPropertyDirective, grex_property_directive, GREX,
                         PROPERTY_DIRECTIVE, GrexDirective)

struct _GrexPropertyDirectiveClass {
  GrexDirectiveClass parent_class;

  void (*attach)(GrexPropertyDirective *directive, GrexFragmentHost *host);
  void (*update)(GrexPropertyDirective *directive, GrexFragmentHost *host);
  void (*detach)(GrexPropertyDirective *directive, GrexFragmentHost *host);

  gpointer padding[13];
};

#define GREX_TYPE_PROPERTY_DIRECTIVE_FACTORY \
  grex_property_directive_factory_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexPropertyDirectiveFactory,
                         grex_property_directive_factory, GREX,
                         PROPERTY_DIRECTIVE_FACTORY, GrexDirectiveFactory)

struct _GrexPropertyDirectiveFactoryClass {
  GrexDirectiveFactoryClass parent_class;

  gboolean (*should_auto_attach)(GrexPropertyDirectiveFactory *factory,
                                 GrexFragmentHost *host,
                                 GrexFragment *fragment);

  GrexPropertyDirective *(*create)(GrexPropertyDirectiveFactory *factory);

  gpointer padding[15];
};

gboolean grex_property_directive_factory_should_auto_attach(
    GrexPropertyDirectiveFactory *factory, GrexFragmentHost *host,
    GrexFragment *fragment);

GrexPropertyDirective *
grex_property_directive_factory_create(GrexPropertyDirectiveFactory *factory);
