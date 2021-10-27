/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-child-property-container-adapter.h"

struct _GrexChildPropertyContainerAdapter {
  GrexContainerAdapter parent_instance;
};

G_DEFINE_TYPE(GrexChildPropertyContainerAdapter,
              grex_child_property_container_adapter,
              GREX_TYPE_CONTAINER_ADAPTER)

static void
grex_child_property_container_adapter_insert_at_front(
    GrexContainerAdapter *adapter, GObject *container, GObject *child) {
  g_return_if_fail(GREX_IS_CHILD_PROPERTY_CONTAINER_ADAPTER(adapter));

  g_auto(GValue) value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_OBJECT);
  g_value_set_object(&value, child);
  g_object_set_property(container, "child", &value);
}

static void
grex_child_property_container_adapter_insert_next_to(
    GrexContainerAdapter *adapter, GObject *container, GObject *child,
    GObject *sibling) {
  g_return_if_fail(GREX_IS_CHILD_PROPERTY_CONTAINER_ADAPTER(adapter));

  g_warning(
      "GrexChildPropertyContainerAdapter can only have a single child widget");
}

static void
grex_child_property_container_adapter_remove(GrexContainerAdapter *adapter,
                                             GObject *container,
                                             GObject *child) {
  g_return_if_fail(GREX_IS_CHILD_PROPERTY_CONTAINER_ADAPTER(adapter));

  g_auto(GValue) value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_OBJECT);
  g_object_get_property(container, "child", &value);

  if (g_value_get_object(&value) != child) {
    g_warning("Child object is not actually the child value");
    return;
  }

  g_value_set_object(&value, NULL);
  g_object_set_property(container, "child", &value);
}

static void
grex_child_property_container_adapter_class_init(
    GrexChildPropertyContainerAdapterClass *klass) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_CLASS(klass);
  adapter_class->insert_at_front =
      grex_child_property_container_adapter_insert_at_front;
  adapter_class->insert_next_to =
      grex_child_property_container_adapter_insert_next_to;
  adapter_class->remove = grex_child_property_container_adapter_remove;
}

static void
grex_child_property_container_adapter_init(
    GrexChildPropertyContainerAdapter *adapter) {}

GrexContainerAdapter *
grex_child_property_container_adapter_new() {
  return g_object_new(GREX_TYPE_CHILD_PROPERTY_CONTAINER_ADAPTER, NULL);
}

struct _GrexChildPropertyContainerDirective {
  GrexPropertyDirective parent_instance;
};

G_DEFINE_TYPE(GrexChildPropertyContainerDirective,
              grex_child_property_container_directive,
              GREX_TYPE_PROPERTY_DIRECTIVE)

static void
grex_child_property_container_directive_attach(GrexPropertyDirective *directive,
                                               GrexFragmentHost *host) {
  g_autoptr(GrexContainerAdapter) adapter =
      grex_child_property_container_adapter_new();
  grex_fragment_host_set_container_adapter(host, adapter);
}

static void
grex_child_property_container_directive_class_init(
    GrexChildPropertyContainerDirectiveClass *klass) {
  GrexPropertyDirectiveClass *property_directive_class =
      GREX_PROPERTY_DIRECTIVE_CLASS(klass);
  property_directive_class->attach =
      grex_child_property_container_directive_attach;
  // TODO: figure out how we want detaching to work
}

static void
grex_child_property_container_directive_init(
    GrexChildPropertyContainerDirective *directive) {}

struct _GrexChildPropertyContainerDirectiveFactory {
  GrexPropertyDirectiveFactory parent_instance;
};

G_DEFINE_TYPE(GrexChildPropertyContainerDirectiveFactory,
              grex_child_property_container_directive_factory,
              GREX_TYPE_PROPERTY_DIRECTIVE_FACTORY)

static const char *
grex_child_property_container_directive_factory_get_name(
    GrexDirectiveFactory *factory) {
  return "grex.child-property-container";
}

static GrexDirectivePropertyFormat
grex_child_property_container_directive_factory_get_property_format(
    GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_NONE;
}

static GrexPropertyDirective *
grex_child_property_container_directive_factory_create(
    GrexPropertyDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_CHILD_PROPERTY_CONTAINER_DIRECTIVE, NULL);
}

static void
grex_child_property_container_directive_factory_class_init(
    GrexChildPropertyContainerDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *factory_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);

  factory_class->get_name =
      grex_child_property_container_directive_factory_get_name;
  factory_class->get_property_format =
      grex_child_property_container_directive_factory_get_property_format;

  GrexPropertyDirectiveFactoryClass *prop_factory_class =
      GREX_PROPERTY_DIRECTIVE_FACTORY_CLASS(klass);
  prop_factory_class->create =
      grex_child_property_container_directive_factory_create;
}

static void
grex_child_property_container_directive_factory_init(
    GrexChildPropertyContainerDirectiveFactory *factory) {}

GrexChildPropertyContainerDirectiveFactory *
grex_child_property_container_directive_factory_new() {
  return g_object_new(GREX_TYPE_CHILD_PROPERTY_CONTAINER_DIRECTIVE_FACTORY,
                      NULL);
}
