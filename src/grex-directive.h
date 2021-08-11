/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"

#define GREX_TYPE_DIRECTIVE grex_directive_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexDirective, grex_directive, GREX, DIRECTIVE,
                         GObject)

typedef struct _GrexDirectiveClassPrivate GrexDirectiveClassPrivate;

struct _GrexDirectiveClass {
  GObjectClass parent_class;

  GrexDirectiveClassPrivate *priv;

  gpointer padding[16];
};

const char *grex_directive_class_get_name(GrexDirectiveClass *klass);
GType grex_directive_class_get_auto_attach(GrexDirectiveClass *klass);

void grex_directive_class_set_name(GrexDirectiveClass *klass, const char *name);
void grex_directive_class_set_auto_attach(GrexDirectiveClass *klass,
                                          GType auto_attach_type);
