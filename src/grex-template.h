/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment.h"
#include "grex-inflator.h"
#include "grex-reactive-inflator.h"
#include "grex-resource-loader.h"

#define GREX_TYPE_TEMPLATE grex_template_get_type()
G_DECLARE_FINAL_TYPE(GrexTemplate, grex_template, GREX, TEMPLATE, GObject)

GrexTemplate *grex_template_new(GrexFragment *fragment, GtkBuilderScope *scope,
                                GrexResourceLoader *loader);

GrexTemplate *grex_template_new_from_xml(const char *xml, gssize len,
                                         const char *filename,
                                         GtkBuilderScope *scope,
                                         GrexResourceLoader *loader);
GrexTemplate *grex_template_new_from_resource(const char *resource,
                                              GtkBuilderScope *scope,
                                              GrexResourceLoader *loader);

GrexFragment *grex_template_get_fragment(GrexTemplate *template);

GrexReactiveInflator *grex_template_create_inflator(GrexTemplate *template,
                                                    GObject *target);
