/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-container-adapter.h"

G_DEFINE_ABSTRACT_TYPE(GrexContainerAdapter, grex_container_adapter,
                       G_TYPE_OBJECT)

static void
grex_container_adapter_class_init(GrexContainerAdapterClass *klass) {}

static void
grex_container_adapter_init(GrexContainerAdapter *adapter) {}

void
grex_container_adapter_insert_at_front(GrexContainerAdapter *adapter,
                                       GObject *container, GObject *child) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_GET_CLASS(adapter);
  g_return_if_fail(adapter_class->insert_at_front != NULL);
  adapter_class->insert_at_front(adapter, container, child);
}

void
grex_container_adapter_insert_next_to(GrexContainerAdapter *adapter,
                                      GObject *container, GObject *child,
                                      GObject *sibling) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_GET_CLASS(adapter);
  g_return_if_fail(adapter_class->insert_next_to != NULL);
  adapter_class->insert_next_to(adapter, container, child, sibling);
}

void
grex_container_adapter_remove(GrexContainerAdapter *adapter, GObject *container,
                              GObject *child) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_GET_CLASS(adapter);
  g_return_if_fail(adapter_class->remove != NULL);
  adapter_class->remove(adapter, container, child);
}
