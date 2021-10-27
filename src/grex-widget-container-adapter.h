/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-container-adapter.h"
#include "grex-property-directive.h"

G_BEGIN_DECLS

#define GREX_TYPE_WIDGET_CONTAINER_ADAPTER \
  grex_widget_container_adapter_get_type()
G_DECLARE_FINAL_TYPE(GrexWidgetContainerAdapter, grex_widget_container_adapter,
                     GREX, WIDGET_CONTAINER_ADAPTER, GrexContainerAdapter)

GrexContainerAdapter *grex_widget_container_adapter_new();

#define GREX_TYPE_WIDGET_CONTAINER_DIRECTIVE \
  grex_widget_container_directive_get_type()
G_DECLARE_FINAL_TYPE(GrexWidgetContainerDirective,
                     grex_widget_container_directive, GREX,
                     WIDGET_CONTAINER_DIRECTIVE, GrexPropertyDirective)

#define GREX_TYPE_WIDGET_CONTAINER_DIRECTIVE_FACTORY \
  grex_widget_container_directive_factory_get_type()
G_DECLARE_FINAL_TYPE(GrexWidgetContainerDirectiveFactory,
                     grex_widget_container_directive_factory, GREX,
                     WIDGET_CONTAINER_DIRECTIVE_FACTORY,
                     GrexPropertyDirectiveFactory)

GrexWidgetContainerDirectiveFactory *
grex_widget_container_directive_factory_new();

G_END_DECLS
