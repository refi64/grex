/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-fragment.h"
#include "grex-inflator.h"

G_BEGIN_DECLS

#define GREX_TYPE_REACTIVE_INFLATOR grex_reactive_inflator_get_type()
G_DECLARE_FINAL_TYPE(GrexReactiveInflator, grex_reactive_inflator, GREX,
                     REACTIVE_INFLATOR, GObject)

GrexReactiveInflator *grex_reactive_inflator_new(GrexFragment *fragment,
                                                 GtkWidget *widget);
GrexReactiveInflator *grex_reactive_inflator_new_with_base_inflator(
    GrexInflator *base_inflator, GrexFragment *fragment, GtkWidget *widget);

GrexInflator *
grex_reactive_inflator_get_base_inflator(GrexReactiveInflator *inflator);
GrexFragment *
grex_reactive_inflator_get_fragment(GrexReactiveInflator *inflator);
GtkWidget *grex_reactive_inflator_get_widget(GrexReactiveInflator *inflator);

void grex_reactive_inflator_inflate(GrexReactiveInflator *inflator);

G_END_DECLS
