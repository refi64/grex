/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-gtk-widget-container-adapter.h"

struct _GrexGtkWidgetContainerAdapter {
  GrexContainerAdapter parent_instance;
};

G_DEFINE_TYPE(GrexGtkWidgetContainerAdapter, grex_gtk_widget_container_adapter,
              GREX_TYPE_CONTAINER_ADAPTER)

static gboolean
is_already_at_position(GtkWidget *container, GtkWidget *child,
                       GtkWidget *sibling) {
  return gtk_widget_get_parent(child) == container &&
         gtk_widget_get_prev_sibling(child) == sibling;
}

static void
grex_gtk_widget_container_adapter_insert_at_front(GrexContainerAdapter *adapter,
                                                  GObject *container,
                                                  GObject *child) {
  g_return_if_fail(GREX_IS_WIDGET_CONTAINER_ADAPTER(adapter));

  GtkWidget *container_widget = GTK_WIDGET(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(container_widget != NULL && child_widget != NULL);

  if (!is_already_at_position(container_widget, child_widget, NULL)) {
    gtk_widget_insert_after(child_widget, container_widget, NULL);
  }
}

static void
grex_gtk_widget_container_adapter_insert_next_to(GrexContainerAdapter *adapter,
                                                 GObject *container,
                                                 GObject *child,
                                                 GObject *sibling) {
  g_return_if_fail(GREX_IS_WIDGET_CONTAINER_ADAPTER(adapter));

  GtkWidget *container_widget = GTK_WIDGET(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  GtkWidget *sibling_widget = GTK_WIDGET(sibling);
  g_return_if_fail(container_widget != NULL && child_widget != NULL &&
                   sibling_widget != NULL);

  if (!is_already_at_position(container_widget, child_widget, sibling_widget)) {
    gtk_widget_insert_after(child_widget, container_widget, sibling_widget);
  }
}

static void
grex_gtk_widget_container_adapter_remove(GrexContainerAdapter *adapter,
                                         GObject *container, GObject *child) {
  g_return_if_fail(GREX_IS_WIDGET_CONTAINER_ADAPTER(adapter));

  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(child_widget != NULL);

  gtk_widget_unparent(child_widget);
}

static void
grex_gtk_widget_container_adapter_class_init(
    GrexGtkWidgetContainerAdapterClass *klass) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_CLASS(klass);
  adapter_class->insert_at_front =
      grex_gtk_widget_container_adapter_insert_at_front;
  adapter_class->insert_next_to =
      grex_gtk_widget_container_adapter_insert_next_to;
  adapter_class->remove = grex_gtk_widget_container_adapter_remove;
}

static void
grex_gtk_widget_container_adapter_init(GrexGtkWidgetContainerAdapter *adapter) {
}

GrexContainerAdapter *
grex_gtk_widget_container_adapter_new() {
  return g_object_new(GREX_TYPE_WIDGET_CONTAINER_ADAPTER, NULL);
}

struct _GrexGtkWidgetContainerDirective {
  GrexPropertyDirective parent_instance;
};

G_DEFINE_TYPE(GrexGtkWidgetContainerDirective,
              grex_gtk_widget_container_directive, GREX_TYPE_PROPERTY_DIRECTIVE)

static void
grex_gtk_widget_container_directive_attach(GrexPropertyDirective *directive,
                                           GrexFragmentHost *host) {
  g_autoptr(GrexContainerAdapter) adapter =
      grex_gtk_widget_container_adapter_new();
  grex_fragment_host_set_container_adapter(host, adapter);
}

static void
grex_gtk_widget_container_directive_class_init(
    GrexGtkWidgetContainerDirectiveClass *klass) {
  GrexPropertyDirectiveClass *property_directive_class =
      GREX_PROPERTY_DIRECTIVE_CLASS(klass);
  property_directive_class->attach = grex_gtk_widget_container_directive_attach;
  // TODO: figure out how we want detaching to work
}

static void
grex_gtk_widget_container_directive_init(
    GrexGtkWidgetContainerDirective *directive) {}

struct _GrexGtkWidgetContainerDirectiveFactory {
  GrexPropertyDirectiveFactory parent_instance;
};

G_DEFINE_TYPE(GrexGtkWidgetContainerDirectiveFactory,
              grex_gtk_widget_container_directive_factory,
              GREX_TYPE_PROPERTY_DIRECTIVE_FACTORY)

static const char *
grex_gtk_widget_container_directive_factory_get_name(
    GrexDirectiveFactory *factory) {
  return "Gtk.widget-container";
}

static GrexDirectivePropertyFormat
grex_gtk_widget_container_directive_factory_get_property_format(
    GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_NONE;
}

static GrexPropertyDirective *
grex_gtk_widget_container_directive_factory_create(
    GrexPropertyDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_WIDGET_CONTAINER_DIRECTIVE, NULL);
}

static void
grex_gtk_widget_container_directive_factory_class_init(
    GrexGtkWidgetContainerDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *factory_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);
  factory_class->get_name =
      grex_gtk_widget_container_directive_factory_get_name;
  factory_class->get_property_format =
      grex_gtk_widget_container_directive_factory_get_property_format;

  GrexPropertyDirectiveFactoryClass *prop_factory_class =
      GREX_PROPERTY_DIRECTIVE_FACTORY_CLASS(klass);
  prop_factory_class->create =
      grex_gtk_widget_container_directive_factory_create;
}

static void
grex_gtk_widget_container_directive_factory_init(
    GrexGtkWidgetContainerDirectiveFactory *factory) {}

GrexGtkWidgetContainerDirectiveFactory *
grex_gtk_widget_container_directive_factory_new() {
  return g_object_new(GREX_TYPE_WIDGET_CONTAINER_DIRECTIVE_FACTORY, NULL);
}
