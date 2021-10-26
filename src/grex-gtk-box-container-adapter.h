/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-container-adapter.h"
#include "grex-property-directive.h"

G_BEGIN_DECLS

#define GREX_TYPE_GTK_BOX_CONTAINER_ADAPTER \
  grex_gtk_box_container_adapter_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkBoxContainerAdapter, grex_gtk_box_container_adapter,
                     GREX, GTK_BOX_CONTAINER_ADAPTER, GrexContainerAdapter)

GrexContainerAdapter *grex_gtk_box_container_adapter_new();

#define GREX_TYPE_GTK_BOX_CONTAINER_ADAPTER_DIRECTIVE \
  grex_gtk_box_container_adapter_directive_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkBoxContainerAdapterDirective,
                     grex_gtk_box_container_adapter_directive, GREX,
                     GTK_BOX_CONTAINER_ADAPTER_DIRECTIVE, GrexPropertyDirective)

#define GREX_TYPE_GTK_BOX_CONTAINER_ADAPTER_DIRECTIVE_FACTORY \
  grex_gtk_box_container_adapter_directive_factory_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkBoxContainerAdapterDirectiveFactory,
                     grex_gtk_box_container_adapter_directive_factory, GREX,
                     GTK_BOX_CONTAINER_ADAPTER_DIRECTIVE_FACTORY,
                     GrexPropertyDirectiveFactory)

GrexGtkBoxContainerAdapterDirectiveFactory *
grex_gtk_box_container_adapter_directive_factory_new();

G_END_DECLS
