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

void grex_prefix_error_with_location(GError **error,
                                     GrexSourceLocation *location);

void grex_set_located_error(GError **error, GrexSourceLocation *location,
                            GQuark domain, int code, const char *format, ...)
    G_GNUC_PRINTF(5, 6);
void grex_set_located_error_va(GError **error, GrexSourceLocation *location,
                               GQuark domain, int code, const char *format,
                               va_list va);

G_END_DECLS
