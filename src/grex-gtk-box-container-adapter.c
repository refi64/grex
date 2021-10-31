/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-gtk-box-container-adapter.h"

struct _GrexGtkBoxContainerAdapter {
  GrexContainerAdapter parent_instance;
};

G_DEFINE_TYPE(GrexGtkBoxContainerAdapter, grex_gtk_box_container_adapter,
              GREX_TYPE_CONTAINER_ADAPTER)

static void
insert_next_to(GtkBox *box, GtkWidget *child, GtkWidget *sibling) {
  GtkWidget *current_parent = gtk_widget_get_parent(child);
  if (current_parent == GTK_WIDGET(box)) {
    // XXX: Technically only widget implementations are supposed to use this.
    if (gtk_widget_get_prev_sibling(child) != sibling) {
      gtk_box_reorder_child_after(box, child, sibling);
    }
  } else if (current_parent == NULL) {
    gtk_box_insert_child_after(box, child, sibling);
  } else {
    g_warning("Child %p already has parent", child);
  }
}

static void
grex_gtk_box_container_adapter_insert_at_front(GrexContainerAdapter *adapter,
                                               GObject *container,
                                               GObject *child) {
  g_return_if_fail(GREX_IS_GTK_BOX_CONTAINER_ADAPTER(adapter));

  GtkBox *container_box = GTK_BOX(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(container_box != NULL && child_widget != NULL);

  insert_next_to(container_box, child_widget, NULL);
}

static void
grex_gtk_box_container_adapter_insert_next_to(GrexContainerAdapter *adapter,
                                              GObject *container,
                                              GObject *child,
                                              GObject *sibling) {
  g_return_if_fail(GREX_IS_GTK_BOX_CONTAINER_ADAPTER(adapter));

  GtkBox *container_box = GTK_BOX(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  GtkWidget *sibling_widget = GTK_WIDGET(sibling);
  g_return_if_fail(container_box != NULL && child_widget != NULL &&
                   sibling_widget != NULL);

  insert_next_to(container_box, child_widget, sibling_widget);
}

static void
grex_gtk_box_container_adapter_remove(GrexContainerAdapter *adapter,
                                      GObject *container, GObject *child) {
  g_return_if_fail(GREX_IS_GTK_BOX_CONTAINER_ADAPTER(adapter));

  GtkBox *container_box = GTK_BOX(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(container_box != NULL && child_widget != NULL);

  gtk_box_remove(container_box, child_widget);
}

static void
grex_gtk_box_container_adapter_class_init(
    GrexGtkBoxContainerAdapterClass *klass) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_CLASS(klass);
  adapter_class->insert_at_front =
      grex_gtk_box_container_adapter_insert_at_front;
  adapter_class->insert_next_to = grex_gtk_box_container_adapter_insert_next_to;
  adapter_class->remove = grex_gtk_box_container_adapter_remove;
}

static void
grex_gtk_box_container_adapter_init(GrexGtkBoxContainerAdapter *adapter) {}

GrexContainerAdapter *
grex_gtk_box_container_adapter_new() {
  return g_object_new(GREX_TYPE_GTK_BOX_CONTAINER_ADAPTER, NULL);
}

struct _GrexGtkBoxContainerDirective {
  GrexPropertyDirective parent_instance;
};

G_DEFINE_TYPE(GrexGtkBoxContainerDirective, grex_gtk_box_container_directive,
              GREX_TYPE_PROPERTY_DIRECTIVE)

static void
grex_gtk_box_container_directive_attach(GrexPropertyDirective *directive,
                                        GrexFragmentHost *host) {
  g_autoptr(GrexContainerAdapter) adapter =
      grex_gtk_box_container_adapter_new();
  grex_fragment_host_set_container_adapter(host, adapter);
}

static void
grex_gtk_box_container_directive_class_init(
    GrexGtkBoxContainerDirectiveClass *klass) {
  GrexPropertyDirectiveClass *property_directive_class =
      GREX_PROPERTY_DIRECTIVE_CLASS(klass);
  property_directive_class->attach = grex_gtk_box_container_directive_attach;
  // TODO: figure out how we want detaching to work
}

static void
grex_gtk_box_container_directive_init(GrexGtkBoxContainerDirective *directive) {
}

struct _GrexGtkBoxContainerDirectiveFactory {
  GrexPropertyDirectiveFactory parent_instance;
};

G_DEFINE_TYPE(GrexGtkBoxContainerDirectiveFactory,
              grex_gtk_box_container_directive_factory,
              GREX_TYPE_PROPERTY_DIRECTIVE_FACTORY)

static const char *
grex_gtk_box_container_directive_factory_get_name(
    GrexDirectiveFactory *factory) {
  return "Gtk.box-container";
}

static GrexDirectivePropertyFormat
grex_gtk_box_container_directive_factory_get_property_format(
    GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_NONE;
}

static GrexPropertyDirective *
grex_gtk_box_container_directive_factory_create(
    GrexPropertyDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_GTK_BOX_CONTAINER_DIRECTIVE, NULL);
}

static gboolean
grex_gtk_box_container_directive_factory_should_auto_attach(
    GrexPropertyDirectiveFactory *factory, GrexFragmentHost *host,
    GrexFragment *fragment) {
  return g_type_is_a(grex_fragment_get_target_type(fragment), GTK_TYPE_BOX);
}

static void
grex_gtk_box_container_directive_factory_class_init(
    GrexGtkBoxContainerDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *factory_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);
  factory_class->get_name = grex_gtk_box_container_directive_factory_get_name;
  factory_class->get_property_format =
      grex_gtk_box_container_directive_factory_get_property_format;

  GrexPropertyDirectiveFactoryClass *prop_factory_class =
      GREX_PROPERTY_DIRECTIVE_FACTORY_CLASS(klass);
  prop_factory_class->create = grex_gtk_box_container_directive_factory_create;
  prop_factory_class->should_auto_attach =
      grex_gtk_box_container_directive_factory_should_auto_attach;
}

static void
grex_gtk_box_container_directive_factory_init(
    GrexGtkBoxContainerDirectiveFactory *factory) {}

GrexGtkBoxContainerDirectiveFactory *
grex_gtk_box_container_directive_factory_new() {
  return g_object_new(GREX_TYPE_GTK_BOX_CONTAINER_DIRECTIVE_FACTORY, NULL);
}
