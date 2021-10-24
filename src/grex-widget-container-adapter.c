/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-widget-container-adapter.h"

struct _GrexWidgetContainerAdapter {
  GrexContainerAdapter parent_instance;
};

G_DEFINE_TYPE(GrexWidgetContainerAdapter, grex_widget_container_adapter,
              GREX_TYPE_CONTAINER_ADAPTER)

static void
grex_widget_container_adapter_insert_at_front(GrexContainerAdapter *adapter,
                                              GObject *container,
                                              GObject *child) {
  g_return_if_fail(GREX_IS_WIDGET_CONTAINER_ADAPTER(adapter));

  GtkWidget *container_widget = GTK_WIDGET(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(container_widget != NULL && child_widget != NULL);

  gtk_widget_insert_after(child_widget, container_widget, NULL);
}

static void
grex_widget_container_adapter_insert_next_to(GrexContainerAdapter *adapter,
                                             GObject *container, GObject *child,
                                             GObject *sibling) {
  g_return_if_fail(GREX_IS_WIDGET_CONTAINER_ADAPTER(adapter));

  GtkWidget *container_widget = GTK_WIDGET(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  GtkWidget *sibling_widget = GTK_WIDGET(sibling);
  g_return_if_fail(container_widget != NULL && child_widget != NULL &&
                   sibling_widget != NULL);

  gtk_widget_insert_after(child_widget, container_widget, sibling_widget);
}

static void
grex_widget_container_adapter_remove(GrexContainerAdapter *adapter,
                                     GObject *container, GObject *child) {
  g_return_if_fail(GREX_IS_WIDGET_CONTAINER_ADAPTER(adapter));

  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(child_widget != NULL);

  gtk_widget_unparent(child_widget);
}

static void
grex_widget_container_adapter_class_init(
    GrexWidgetContainerAdapterClass *klass) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_CLASS(klass);
  adapter_class->insert_at_front =
      grex_widget_container_adapter_insert_at_front;
  adapter_class->insert_next_to = grex_widget_container_adapter_insert_next_to;
  adapter_class->remove = grex_widget_container_adapter_remove;
}

static void
grex_widget_container_adapter_init(GrexWidgetContainerAdapter *adapter) {}

GrexContainerAdapter *
grex_widget_container_adapter_new() {
  return g_object_new(GREX_TYPE_WIDGET_CONTAINER_ADAPTER, NULL);
}

struct _GrexWidgetContainerAdapterDirective {
  GrexAttributeDirective parent_instance;
};

G_DEFINE_TYPE(GrexWidgetContainerAdapterDirective,
              grex_widget_container_adapter_directive,
              GREX_TYPE_ATTRIBUTE_DIRECTIVE)

static void
grex_widget_container_adapter_directive_attach(
    GrexAttributeDirective *directive, GrexFragmentHost *host) {
  g_autoptr(GrexContainerAdapter) adapter = grex_widget_container_adapter_new();
  grex_fragment_host_set_container_adapter(host, adapter);
}

static void
grex_widget_container_adapter_directive_class_init(
    GrexWidgetContainerAdapterDirectiveClass *klass) {
  GrexAttributeDirectiveClass *attr_directive_class =
      GREX_ATTRIBUTE_DIRECTIVE_CLASS(klass);
  attr_directive_class->attach = grex_widget_container_adapter_directive_attach;
  // TODO: figure out how we want detaching to work
}

static void
grex_widget_container_adapter_directive_init(
    GrexWidgetContainerAdapterDirective *directive) {}

struct _GrexWidgetContainerAdapterDirectiveFactory {
  GrexDirectiveFactory parent_instance;
};

G_DEFINE_TYPE(GrexWidgetContainerAdapterDirectiveFactory,
              grex_widget_container_adapter_directive_factory,
              GREX_TYPE_DIRECTIVE_FACTORY)

static const char *
grex_widget_container_adapter_directive_factory_get_name(
    GrexDirectiveFactory *factory) {
  return "grex.widget-container-adapter";
}

static GrexDirectivePropertyFormat
grex_widget_container_adapter_directive_factory_get_property_format(
    GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_NONE;
}

static GrexDirective *
grex_widget_container_adapter_directive_factory_create(
    GrexDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_WIDGET_CONTAINER_ADAPTER_DIRECTIVE, NULL);
}

static void
grex_widget_container_adapter_directive_factory_class_init(
    GrexWidgetContainerAdapterDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *factory_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);
  factory_class->get_name =
      grex_widget_container_adapter_directive_factory_get_name;
  factory_class->get_property_format =
      grex_widget_container_adapter_directive_factory_get_property_format;
  factory_class->create =
      grex_widget_container_adapter_directive_factory_create;
}

static void
grex_widget_container_adapter_directive_factory_init(
    GrexWidgetContainerAdapterDirectiveFactory *factory) {}
