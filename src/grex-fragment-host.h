/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-container-adapter.h"
#include "grex-fragment.h"

G_BEGIN_DECLS

typedef struct _GrexAttributeDirective GrexAttributeDirective;

#define GREX_TYPE_FRAGMENT_HOST grex_fragment_host_get_type()
G_DECLARE_FINAL_TYPE(GrexFragmentHost, grex_fragment_host, GREX, FRAGMENT_HOST,
                     GObject)

GrexFragmentHost *grex_fragment_host_new(GObject *target);

GrexFragmentHost *grex_fragment_host_for_target(GObject *target);

GrexContainerAdapter *
grex_fragment_host_get_container_adapter(GrexFragmentHost *host);
void grex_fragment_host_set_container_adapter(GrexFragmentHost *host,
                                              GrexContainerAdapter *adapter);

GObject *grex_fragment_host_get_target(GrexFragmentHost *host);

gboolean grex_fragment_host_matches_fragment_type(GrexFragmentHost *host,
                                                  GrexFragment *fragment);

void grex_fragment_host_begin_inflation(GrexFragmentHost *host);

void grex_fragment_host_add_property(GrexFragmentHost *host, const char *name,
                                     GrexValueHolder *value);

GrexAttributeDirective *
grex_fragment_host_get_leftover_attribute_directive(GrexFragmentHost *host,
                                                    guintptr key);
void
grex_fragment_host_add_attribute_directive(GrexFragmentHost *host, guintptr key,
                                           GrexAttributeDirective *directive);

void grex_fragment_host_apply_pending_directive_updates(GrexFragmentHost *host);

GObject *grex_fragment_host_get_leftover_child(GrexFragmentHost *host,
                                               guintptr key);
void grex_fragment_host_add_inflated_child(GrexFragmentHost *host, guintptr key,
                                           GObject *child);

void grex_fragment_host_commit_inflation(GrexFragmentHost *host);

G_END_DECLS
