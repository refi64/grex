/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment.h"
#include "grex-property-set.h"

G_BEGIN_DECLS

typedef struct _GrexAttributeDirective GrexAttributeDirective;

#define GREX_TYPE_FRAGMENT_HOST grex_fragment_host_get_type()
G_DECLARE_FINAL_TYPE(GrexFragmentHost, grex_fragment_host, GREX, FRAGMENT_HOST,
                     GObject)

GrexFragmentHost *grex_fragment_host_new(GtkWidget *widget);

GrexFragmentHost *grex_fragment_host_for_widget(GtkWidget *widget);

GrexPropertySet *
grex_fragment_host_get_applied_properties(GrexFragmentHost *host);
GtkWidget *grex_fragment_host_get_widget(GrexFragmentHost *host);

gboolean grex_fragment_host_matches_fragment_type(GrexFragmentHost *host,
                                                  GrexFragment *fragment);

void grex_fragment_host_apply_latest_properties(GrexFragmentHost *host,
                                                GrexPropertySet *properties);

void grex_fragment_host_begin_inflation(GrexFragmentHost *host);

GrexAttributeDirective *
grex_fragment_host_get_leftover_attribute_directive(GrexFragmentHost *host,
                                                    GType type);
void
grex_fragment_host_add_attribute_directive(GrexFragmentHost *host,
                                           GrexAttributeDirective *directive);

void grex_fragment_host_apply_pending_directive_updates(GrexFragmentHost *host);

GtkWidget *grex_fragment_host_get_leftover_child(GrexFragmentHost *host,
                                                 guintptr key);
void grex_fragment_host_add_inflated_child(GrexFragmentHost *host, guintptr key,
                                           GtkWidget *child);

void grex_fragment_host_commit_inflation(GrexFragmentHost *host);

G_END_DECLS
