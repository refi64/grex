/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-container-adapter.h"
#include "grex-property-directive.h"

G_BEGIN_DECLS

#define GREX_TYPE_GTK_GRID_CONTAINER_ADAPTER \
  grex_gtk_grid_container_adapter_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkGridContainerAdapter,
                     grex_gtk_grid_container_adapter, GREX,
                     GTK_GRID_CONTAINER_ADAPTER, GrexContainerAdapter)

GrexContainerAdapter *grex_gtk_grid_container_adapter_new();

#define GREX_TYPE_GTK_GRID_CONTAINER_DIRECTIVE \
  grex_gtk_grid_container_directive_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkGridContainerDirective,
                     grex_gtk_grid_container_directive, GREX,
                     GTK_GRID_CONTAINER_DIRECTIVE, GrexPropertyDirective)

#define GREX_TYPE_GTK_GRID_CONTAINER_DIRECTIVE_FACTORY \
  grex_gtk_grid_container_directive_factory_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkGridContainerDirectiveFactory,
                     grex_gtk_grid_container_directive_factory, GREX,
                     GTK_GRID_CONTAINER_DIRECTIVE_FACTORY,
                     GrexPropertyDirectiveFactory)

GrexGtkGridContainerDirectiveFactory *
grex_gtk_grid_container_directive_factory_new();

#define GREX_TYPE_GTK_GRID_CHILD_DIRECTIVE \
  grex_gtk_grid_child_directive_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkGridChildDirective, grex_gtk_grid_child_directive,
                     GREX, GTK_GRID_CHILD_DIRECTIVE, GrexPropertyDirective)

#define GREX_TYPE_GTK_GRID_CHILD_DIRECTIVE_FACTORY \
  grex_gtk_grid_child_directive_factory_get_type()
G_DECLARE_FINAL_TYPE(GrexGtkGridChildDirectiveFactory,
                     grex_gtk_grid_child_directive_factory, GREX,
                     GTK_GRID_CHILD_DIRECTIVE_FACTORY,
                     GrexPropertyDirectiveFactory)

GrexGtkGridChildDirectiveFactory *grex_gtk_grid_child_directive_factory_new();

G_END_DECLS
