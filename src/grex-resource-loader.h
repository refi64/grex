/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <gio/gio.h>
#include <glib-object.h>

#define GREX_TYPE_RESOURCE_LOADER grex_resource_loader_get_type()
G_DECLARE_FINAL_TYPE(GrexResourceLoader, grex_resource_loader, GREX,
                     RESOURCE_LOADER, GObject)

GrexResourceLoader *grex_resource_loader_default();

GrexResourceLoader *grex_resource_loader_new(gboolean is_reload_enabled);

gboolean grex_resource_loader_is_reload_enabled(GrexResourceLoader *loader);
gboolean grex_resource_loader_register(GrexResourceLoader *loader,
                                       const char *filename, GError **error);
