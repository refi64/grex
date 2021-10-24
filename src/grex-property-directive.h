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
