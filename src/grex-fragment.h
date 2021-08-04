/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-source-location.h"

G_BEGIN_DECLS

#define GREX_TYPE_FRAGMENT grex_fragment_get_type()
G_DECLARE_FINAL_TYPE(GrexFragment, grex_fragment, GREX, FRAGMENT, GObject)

GrexFragment *grex_fragment_new(GType widget_type,
                                GrexSourceLocation *location);

GrexFragment *grex_fragment_parse_xml(const char *xml, const char *filename,
                                      GtkBuilderScope *scope, GError **error);

GType grex_fragment_get_widget_type(GrexFragment *fragment);
GrexSourceLocation *grex_fragment_get_location(GrexFragment *fragment);

void grex_fragment_add_child(GrexFragment *fragment, GrexFragment *child);
GList *grex_fragment_get_children(GrexFragment *fragment);

G_END_DECLS
