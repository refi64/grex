/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment.h"
#include "grex-property-set.h"

G_BEGIN_DECLS

#define GREX_TYPE_FRAGMENT_HOST grex_fragment_host_get_type()
G_DECLARE_FINAL_TYPE(GrexFragmentHost, grex_fragment_host, GREX, FRAGMENT_HOST,
                     GObject)

GrexFragmentHost *grex_fragment_host_new(GtkWidget *widget);

GrexFragmentHost *grex_fragment_host_for_widget(GtkWidget *widget);

GtkWidget *grex_fragment_host_create_with_widget(GrexFragment *fragment);

GrexPropertySet *
grex_fragment_host_get_applied_properties(GrexFragmentHost *host);
GtkWidget *grex_fragment_host_get_widget(GrexFragmentHost *host);

gboolean grex_fragment_host_matches_fragment_type(GrexFragmentHost *host,
                                                  GrexFragment *fragment);

void grex_fragment_host_apply_latest_properties(GrexFragmentHost *host,
                                                GrexPropertySet *properties);

G_END_DECLS
