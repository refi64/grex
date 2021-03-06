/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-container-adapter.h"
#include "grex-fragment.h"
#include "grex-key.h"

G_BEGIN_DECLS

typedef struct _GrexPropertyDirective GrexPropertyDirective;
typedef struct _GrexStructuralDirective GrexStructuralDirective;

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
void grex_fragment_host_add_signal(GrexFragmentHost *host, GrexKey *key,
                                   const char *signal, GClosure *closure,
                                   gboolean after);

GrexPropertyDirective *
grex_fragment_host_get_leftover_property_directive(GrexFragmentHost *host,
                                                   GrexKey *key);
void
grex_fragment_host_add_property_directive(GrexFragmentHost *host, GrexKey *key,
                                          GrexPropertyDirective *directive);

void grex_fragment_host_apply_pending_directive_updates(GrexFragmentHost *host);

GrexStructuralDirective *
grex_fragment_host_get_leftover_structural_directive(GrexFragmentHost *host,
                                                     GrexKey *key);
void grex_fragment_host_add_structural_directive(
    GrexFragmentHost *host, GrexKey *key, GrexStructuralDirective *directive);

GObject *grex_fragment_host_get_leftover_child(GrexFragmentHost *host,
                                               GrexKey *key);
void grex_fragment_host_add_inflated_child(GrexFragmentHost *host, GrexKey *key,
                                           GObject *child);

void grex_fragment_host_commit_inflation(GrexFragmentHost *host);

G_END_DECLS
