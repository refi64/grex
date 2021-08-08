/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment-host.h"
#include "grex-fragment.h"

G_BEGIN_DECLS

#define GREX_TYPE_INFLATOR grex_inflator_get_type()
G_DECLARE_FINAL_TYPE(GrexInflator, grex_inflator, GREX, INFLATOR, GObject)

GrexInflator *grex_inflator_new();

GtkWidget *grex_inflator_inflate_new_widget(GrexInflator *inflator,
                                            GrexFragment *fragment);
void grex_inflator_inflate_existing_widget(GrexInflator *inflator,
                                           GtkWidget *widget,
                                           GrexFragment *fragment);

void grex_inflator_inflate_child(GrexInflator *inflator,
                                 GrexFragmentHost *parent, guintptr key,
                                 GrexFragment *child);

G_END_DECLS
