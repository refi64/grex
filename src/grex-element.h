/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-source-location.h"

G_BEGIN_DECLS

#define GREX_TYPE_ELEMENT grex_element_get_type()
G_DECLARE_FINAL_TYPE(GrexElement, grex_element, GREX, ELEMENT, GObject)

GrexElement *grex_element_new(GType root_type, GrexSourceLocation *location);

GrexElement *grex_element_parse_xml(const char *xml, const char *filename,
                                    GtkBuilderScope *scope, GError **error);

GType grex_element_get_root_type(GrexElement *element);
GrexSourceLocation *grex_element_get_location(GrexElement *element);

void grex_element_add_child(GrexElement *element, GrexElement *child);
GList *grex_element_get_children(GrexElement *element);

G_END_DECLS
