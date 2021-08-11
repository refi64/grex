/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"

G_BEGIN_DECLS

#define GREX_TYPE_CONTAINER_ADAPTER grex_container_adapter_get_type()
G_DECLARE_DERIVABLE_TYPE(GrexContainerAdapter, grex_container_adapter, GREX,
                         CONTAINER_ADAPTER, GObject)

struct _GrexContainerAdapterClass {
  GObjectClass object_class;

  void (*insert_at_front)(GrexContainerAdapter *adapter, GObject *container,
                          GObject *child);
  void (*insert_next_to)(GrexContainerAdapter *adapter, GObject *container,
                         GObject *child, GObject *sibling);
  void (*remove)(GrexContainerAdapter *adapter, GObject *container,
                 GObject *child);

  gpointer padding[4];
};

void grex_container_adapter_insert_at_front(GrexContainerAdapter *adapter,
                                            GObject *container, GObject *child);
void grex_container_adapter_insert_next_to(GrexContainerAdapter *adapter,
                                           GObject *container, GObject *child,
                                           GObject *sibling);
void grex_container_adapter_remove(GrexContainerAdapter *adapter,
                                   GObject *container, GObject *child);

G_END_DECLS
