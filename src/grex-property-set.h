/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"

G_BEGIN_DECLS

#define GREX_TYPE_PROPERTY_SET grex_property_set_get_type()
G_DECLARE_FINAL_TYPE(GrexPropertySet, grex_property_set, GREX, PROPERTY_SET,
                     GObject)

GrexPropertySet *grex_property_set_new();

GList *grex_property_set_get_keys(GrexPropertySet *properties);
void grex_property_set_add(GrexPropertySet *properties, const char *name,
                           const GValue *value);
gboolean grex_property_set_contains(GrexPropertySet *properties,
                                    const char *name);
gboolean grex_property_set_get(GrexPropertySet *properties, const char *name,
                               GValue *value);
gboolean grex_property_set_remove(GrexPropertySet *properties,
                                  const char *name);

void grex_property_set_diff_keys(GrexPropertySet *old_set,
                                 GrexPropertySet *new_set, GList **added,
                                 GList **removed, GList **kept);

G_END_DECLS
