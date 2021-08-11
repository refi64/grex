/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-attribute-directive.h"
#include "grex-config.h"
#include "grex-container-adapter.h"

G_BEGIN_DECLS

#define GREX_TYPE_WIDGET_CONTAINER_ADAPTER \
  grex_widget_container_adapter_get_type()
G_DECLARE_FINAL_TYPE(GrexWidgetContainerAdapter, grex_widget_container_adapter,
                     GREX, WIDGET_CONTAINER_ADAPTER, GrexContainerAdapter)

GrexContainerAdapter *grex_widget_container_adapter_new();

#define GREX_TYPE_WIDGET_CONTAINER_ADAPTER_DIRECTIVE \
  grex_widget_container_adapter_directive_get_type()
G_DECLARE_FINAL_TYPE(GrexWidgetContainerAdapterDirective,
                     grex_widget_container_adapter_directive, GREX,
                     WIDGET_CONTAINER_ADAPTER_DIRECTIVE, GrexAttributeDirective)

G_END_DECLS
