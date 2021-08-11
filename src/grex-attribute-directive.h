/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-directive.h"
#include "grex-fragment-host.h"

#define GREX_TYPE_ATTRIBUTE_DIRECTIVE grex_attribute_directive_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexAttributeDirective, grex_attribute_directive, GREX,
                         ATTRIBUTE_DIRECTIVE, GrexDirective)

struct _GrexAttributeDirectiveClass {
  GrexDirectiveClass parent_class;

  void (*attach)(GrexAttributeDirective *directive, GrexFragmentHost *host);
  void (*update)(GrexAttributeDirective *directive, GrexFragmentHost *host);
  void (*detach)(GrexAttributeDirective *directive, GrexFragmentHost *host);

  gpointer padding[13];
};
