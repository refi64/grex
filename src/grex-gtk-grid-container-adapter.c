/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-gtk-grid-container-adapter.h"

#include "gpropz.h"

typedef struct {
  guint row, column, row_span, column_span;
  GtkPositionType sibling_side;
} ChildPlacement;

const static ChildPlacement default_child_placement = {.row = -1,
                                                       .column = -1,
                                                       .row_span = 1,
                                                       .column_span = 1,
                                                       .sibling_side =
                                                           GTK_POS_RIGHT};

G_DEFINE_QUARK("grex-gtk-grid-child-placement", grex_gtk_grid_child_placement)

static ChildPlacement
get_child_placement(GObject *child) {
  ChildPlacement *requested =
      g_object_get_qdata(child, grex_gtk_grid_child_placement_quark());
  return requested != NULL ? *requested : default_child_placement;
}

struct _GrexGtkGridContainerAdapter {
  GrexContainerAdapter parent_instance;
};

G_DEFINE_TYPE(GrexGtkGridContainerAdapter, grex_gtk_grid_container_adapter,
              GREX_TYPE_CONTAINER_ADAPTER)

static void
grex_gtk_grid_container_adapter_insert_at_front(GrexContainerAdapter *adapter,
                                                GObject *container,
                                                GObject *child) {
  g_return_if_fail(GREX_IS_GTK_GRID_CONTAINER_ADAPTER(adapter));

  GtkGrid *container_grid = GTK_GRID(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(container_grid != NULL && child_widget != NULL);

  ChildPlacement placement = get_child_placement(child);
  if (placement.row == -1) {
    placement.row = 0;
  }
  if (placement.column == -1) {
    placement.column = 0;
  }

  gtk_grid_attach(container_grid, child_widget, placement.column, placement.row,
                  placement.column_span, placement.row_span);
}

static void
grex_gtk_grid_container_adapter_insert_next_to(GrexContainerAdapter *adapter,
                                               GObject *container,
                                               GObject *child,
                                               GObject *sibling) {
  g_return_if_fail(GREX_IS_GTK_GRID_CONTAINER_ADAPTER(adapter));

  GtkGrid *container_grid = GTK_GRID(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  GtkWidget *sibling_widget = GTK_WIDGET(sibling);
  g_return_if_fail(container_grid != NULL && child_widget != NULL &&
                   sibling_widget != NULL);

  ChildPlacement placement = get_child_placement(child);
  if (placement.row == -1 && placement.column == -1) {
    gtk_grid_attach_next_to(container_grid, child_widget, sibling_widget,
                            placement.sibling_side, placement.column_span,
                            placement.row_span);
  } else {
    // XXX: need to handle this error in a clearer place.
    g_return_if_fail(placement.row != -1 || placement.column != -1);
    gtk_grid_attach(container_grid, child_widget, placement.column,
                    placement.row, placement.column_span, placement.row_span);
  }
}

static void
grex_gtk_grid_container_adapter_remove(GrexContainerAdapter *adapter,
                                       GObject *container, GObject *child) {
  g_return_if_fail(GREX_IS_GTK_GRID_CONTAINER_ADAPTER(adapter));

  GtkGrid *container_grid = GTK_GRID(container);
  GtkWidget *child_widget = GTK_WIDGET(child);
  g_return_if_fail(container_grid != NULL && child_widget != NULL);

  gtk_grid_remove(container_grid, child_widget);
}

static void
grex_gtk_grid_container_adapter_class_init(
    GrexGtkGridContainerAdapterClass *klass) {
  GrexContainerAdapterClass *adapter_class =
      GREX_CONTAINER_ADAPTER_CLASS(klass);
  adapter_class->insert_at_front =
      grex_gtk_grid_container_adapter_insert_at_front;
  adapter_class->insert_next_to =
      grex_gtk_grid_container_adapter_insert_next_to;
  adapter_class->remove = grex_gtk_grid_container_adapter_remove;
}

static void
grex_gtk_grid_container_adapter_init(GrexGtkGridContainerAdapter *adapter) {}

GrexContainerAdapter *
grex_gtk_grid_container_adapter_new() {
  return g_object_new(GREX_TYPE_GTK_GRID_CONTAINER_ADAPTER, NULL);
}

struct _GrexGtkGridContainerDirective {
  GrexPropertyDirective parent_instance;
};

G_DEFINE_TYPE(GrexGtkGridContainerDirective, grex_gtk_grid_container_directive,
              GREX_TYPE_PROPERTY_DIRECTIVE)

static void
grex_gtk_grid_container_directive_attach(GrexPropertyDirective *directive,
                                         GrexFragmentHost *host) {
  g_autoptr(GrexContainerAdapter) adapter =
      grex_gtk_grid_container_adapter_new();
  grex_fragment_host_set_container_adapter(host, adapter);
}

static void
grex_gtk_grid_container_directive_class_init(
    GrexGtkGridContainerDirectiveClass *klass) {
  GrexPropertyDirectiveClass *property_directive_class =
      GREX_PROPERTY_DIRECTIVE_CLASS(klass);
  property_directive_class->attach = grex_gtk_grid_container_directive_attach;
  // TODO: figure out how we want detaching to work
}

static void
grex_gtk_grid_container_directive_init(
    GrexGtkGridContainerDirective *directive) {}

struct _GrexGtkGridContainerDirectiveFactory {
  GrexPropertyDirectiveFactory parent_instance;
};

G_DEFINE_TYPE(GrexGtkGridContainerDirectiveFactory,
              grex_gtk_grid_container_directive_factory,
              GREX_TYPE_PROPERTY_DIRECTIVE_FACTORY)

static const char *
grex_gtk_grid_container_directive_factory_get_name(
    GrexDirectiveFactory *factory) {
  return "Gtk.grid-container";
}

static GrexDirectivePropertyFormat
grex_gtk_grid_container_directive_factory_get_property_format(
    GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_NONE;
}

static GrexPropertyDirective *
grex_gtk_grid_container_directive_factory_create(
    GrexPropertyDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_GTK_GRID_CONTAINER_DIRECTIVE, NULL);
}

static gboolean
grex_gtk_grid_container_directive_factory_should_auto_attach(
    GrexPropertyDirectiveFactory *factory, GrexFragmentHost *host,
    GrexFragment *fragment) {
  return grex_fragment_host_get_container_adapter(host) == NULL &&
         g_type_is_a(grex_fragment_get_target_type(fragment), GTK_TYPE_GRID);
}

static void
grex_gtk_grid_container_directive_factory_class_init(
    GrexGtkGridContainerDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *factory_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);
  factory_class->get_name = grex_gtk_grid_container_directive_factory_get_name;
  factory_class->get_property_format =
      grex_gtk_grid_container_directive_factory_get_property_format;

  GrexPropertyDirectiveFactoryClass *prop_factory_class =
      GREX_PROPERTY_DIRECTIVE_FACTORY_CLASS(klass);
  prop_factory_class->create = grex_gtk_grid_container_directive_factory_create;
  prop_factory_class->should_auto_attach =
      grex_gtk_grid_container_directive_factory_should_auto_attach;
}

static void
grex_gtk_grid_container_directive_factory_init(
    GrexGtkGridContainerDirectiveFactory *factory) {}

GrexGtkGridContainerDirectiveFactory *
grex_gtk_grid_container_directive_factory_new() {
  return g_object_new(GREX_TYPE_GTK_GRID_CONTAINER_DIRECTIVE_FACTORY, NULL);
}

struct _GrexGtkGridChildDirective {
  GrexPropertyDirective parent_instance;

  ChildPlacement placement;
};

enum {
  CHILD_PROP_ROW = 1,
  CHILD_PROP_COLUMN,
  CHILD_PROP_ROW_SPAN,
  CHILD_PROP_COLUMN_SPAN,
  CHILD_PROP_ON_SIDE,
  N_CHILD_PROPS,
};

static GParamSpec *child_properties[N_CHILD_PROPS] = {NULL};

G_DEFINE_TYPE(GrexGtkGridChildDirective, grex_gtk_grid_child_directive,
              GREX_TYPE_PROPERTY_DIRECTIVE)

static void
grex_gtk_grid_child_directive_attach(GrexPropertyDirective *directive,
                                     GrexFragmentHost *host) {}

static void
grex_gtk_grid_child_directive_update(GrexPropertyDirective *directive,
                                     GrexFragmentHost *host) {
  GrexGtkGridChildDirective *child_directive =
      GREX_GTK_GRID_CHILD_DIRECTIVE(directive);
  GObject *target = grex_fragment_host_get_target(host);

  ChildPlacement *location = g_new0(ChildPlacement, 1);
  *location = child_directive->placement;
  g_object_set_qdata_full(target, grex_gtk_grid_child_placement_quark(),
                          location, g_free);
}

static void
grex_gtk_grid_child_directive_detach(GrexPropertyDirective *directive,
                                     GrexFragmentHost *host) {
  GObject *target = grex_fragment_host_get_target(host);
  g_object_set_qdata(target, grex_gtk_grid_child_placement_quark(), NULL);
}

static void
grex_gtk_grid_child_directive_class_init(
    GrexGtkGridChildDirectiveClass *klass) {
  GrexPropertyDirectiveClass *property_directive_class =
      GREX_PROPERTY_DIRECTIVE_CLASS(klass);
  property_directive_class->attach = grex_gtk_grid_child_directive_attach;
  property_directive_class->update = grex_gtk_grid_child_directive_update;
  property_directive_class->detach = grex_gtk_grid_child_directive_detach;

  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  gpropz_class_init_property_functions(object_class);

  child_properties[CHILD_PROP_ROW] =
      g_param_spec_int("row", "Row", "The row of the grid.", -1, G_MAXINT, -1,
                       G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexGtkGridChildDirective,
                          placement.row, CHILD_PROP_ROW,
                          child_properties[CHILD_PROP_ROW], NULL);

  child_properties[CHILD_PROP_ROW_SPAN] =
      g_param_spec_int("row-span", "Row span", "The number of rows to span.", 1,
                       G_MAXINT, 1, G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexGtkGridChildDirective,
                          placement.row_span, CHILD_PROP_ROW_SPAN,
                          child_properties[CHILD_PROP_ROW_SPAN], NULL);

  child_properties[CHILD_PROP_COLUMN] =
      g_param_spec_int("column", "Column", "The column of the grid.", -1,
                       G_MAXINT, -1, G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexGtkGridChildDirective,
                          placement.column, CHILD_PROP_COLUMN,
                          child_properties[CHILD_PROP_COLUMN], NULL);

  child_properties[CHILD_PROP_COLUMN_SPAN] = g_param_spec_int(
      "column-span", "Column span", "The number of columns to span.", 1,
      G_MAXINT, 1, G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexGtkGridChildDirective,
                          placement.column_span, CHILD_PROP_COLUMN_SPAN,
                          child_properties[CHILD_PROP_COLUMN_SPAN], NULL);

  child_properties[CHILD_PROP_ON_SIDE] = g_param_spec_enum(
      "on-side", "On sibling's side",
      "Place on the given side of the previously added sibling.",
      GTK_TYPE_POSITION_TYPE, GTK_POS_LEFT, G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexGtkGridChildDirective,
                          placement.sibling_side, CHILD_PROP_ON_SIDE,
                          child_properties[CHILD_PROP_ON_SIDE], NULL);
}

static void
grex_gtk_grid_child_directive_init(GrexGtkGridChildDirective *directive) {
  directive->placement = default_child_placement;
}

struct _GrexGtkGridChildDirectiveFactory {
  GrexPropertyDirectiveFactory parent_instance;
};

G_DEFINE_TYPE(GrexGtkGridChildDirectiveFactory,
              grex_gtk_grid_child_directive_factory,
              GREX_TYPE_PROPERTY_DIRECTIVE_FACTORY)

static const char *
grex_gtk_grid_child_directive_factory_get_name(GrexDirectiveFactory *factory) {
  return "Gtk.grid-child";
}

static GrexDirectivePropertyFormat
grex_gtk_grid_child_directive_factory_get_property_format(
    GrexDirectiveFactory *factory) {
  return GREX_DIRECTIVE_PROPERTY_FORMAT_EXPLICIT;
}

static GrexPropertyDirective *
grex_gtk_grid_child_directive_factory_create(
    GrexPropertyDirectiveFactory *factory) {
  return g_object_new(GREX_TYPE_GTK_GRID_CHILD_DIRECTIVE, NULL);
}

static gboolean
grex_gtk_grid_child_directive_factory_should_auto_attach(
    GrexPropertyDirectiveFactory *factory, GrexFragmentHost *host,
    GrexFragment *fragment) {
  return grex_fragment_host_get_container_adapter(host) == NULL &&
         g_type_is_a(grex_fragment_get_target_type(fragment), GTK_TYPE_GRID);
}

static void
grex_gtk_grid_child_directive_factory_class_init(
    GrexGtkGridChildDirectiveFactoryClass *klass) {
  GrexDirectiveFactoryClass *factory_class =
      GREX_DIRECTIVE_FACTORY_CLASS(klass);
  factory_class->get_name = grex_gtk_grid_child_directive_factory_get_name;
  factory_class->get_property_format =
      grex_gtk_grid_child_directive_factory_get_property_format;

  GrexPropertyDirectiveFactoryClass *prop_factory_class =
      GREX_PROPERTY_DIRECTIVE_FACTORY_CLASS(klass);
  prop_factory_class->create = grex_gtk_grid_child_directive_factory_create;
  prop_factory_class->should_auto_attach =
      grex_gtk_grid_child_directive_factory_should_auto_attach;
}

static void
grex_gtk_grid_child_directive_factory_init(
    GrexGtkGridChildDirectiveFactory *factory) {}

GrexGtkGridChildDirectiveFactory *
grex_gtk_grid_child_directive_factory_new() {
  return g_object_new(GREX_TYPE_GTK_GRID_CONTAINER_DIRECTIVE_FACTORY, NULL);
}
