/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"

G_BEGIN_DECLS

#define GREX_TYPE_SOURCE_LOCATION grex_source_location_get_type()
G_DECLARE_FINAL_TYPE(GrexSourceLocation, grex_source_location, GREX,
                     SOURCE_LOCATION, GObject)

GrexSourceLocation *grex_source_location_new(const char *file, gint line,
                                             gint column);
GrexSourceLocation *grex_source_location_new_offset(GrexSourceLocation *base,
                                                    gint line_offset,
                                                    gint column_offset);

const char *grex_source_location_get_file(GrexSourceLocation *location);
gint grex_source_location_get_line(GrexSourceLocation *location);
gint grex_source_location_get_column(GrexSourceLocation *location);

char *grex_source_location_format(GrexSourceLocation *location);

G_END_DECLS
