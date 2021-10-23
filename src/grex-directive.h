/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment-host.h"
#include "grex-fragment.h"

#define GREX_TYPE_DIRECTIVE grex_directive_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexDirective, grex_directive, GREX, DIRECTIVE,
                         GObject)

typedef struct _GrexDirectiveClassPrivate GrexDirectiveClassPrivate;

struct _GrexDirectiveClass {
  GObjectClass parent_class;

  gpointer padding[16];
};

#define GREX_TYPE_DIRECTIVE_FACTORY grex_directive_factory_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexDirectiveFactory, grex_directive_factory, GREX,
                         DIRECTIVE_FACTORY, GObject)

struct _GrexDirectiveFactoryClass {
  GObjectClass parent_class;

  const char *(*get_name)(GrexDirectiveFactory *factory);

  GrexDirective *(*create)(GrexDirectiveFactory *factory);

  gboolean (*should_auto_attach)(GrexDirectiveFactory *factory,
                                 GrexFragmentHost *host,
                                 GrexFragment *fragment);

  gpointer padding[12];
};

const char *grex_directive_factory_get_name(GrexDirectiveFactory *factory);
GrexDirective *grex_directive_factory_create(GrexDirectiveFactory *factory);

gboolean
grex_directive_factory_should_auto_attach(GrexDirectiveFactory *factory,
                                          GrexFragmentHost *host,
                                          GrexFragment *fragment);
