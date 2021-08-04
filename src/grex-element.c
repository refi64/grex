/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-element.h"

#include "gpropz.h"

/*
 * GrexElement:
 *
 * TODO: docs
 */
struct _GrexElement {
  GObject parent_instance;

  GType root_type;
  GrexSourceLocation *location;

  GPtrArray *children;
};

typedef struct {
  GrexSourceLocation *location;
} GrexElementParseErrorPrivate;

enum {
  PROP_ROOT_TYPE = 1,
  PROP_LOCATION,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexElement, grex_element, G_TYPE_OBJECT)

static void
grex_element_dispose(GObject *object) {
  GrexElement *element = GREX_ELEMENT(object);

  g_clear_object(&element->location);                      // NOLINT
  g_clear_pointer(&element->children, g_ptr_array_unref);  // NOLINT
}

static void
grex_element_class_init(GrexElementClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_element_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_ROOT_TYPE] = g_param_spec_gtype(
      "root-type", "Root type", "The type this element represents.",
      G_TYPE_NONE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexElement, root_type, PROP_ROOT_TYPE,
                          properties[PROP_ROOT_TYPE], NULL);

  properties[PROP_LOCATION] = g_param_spec_object(
      "location", "Source location",
      "The source location this element is from.", GREX_TYPE_SOURCE_LOCATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexElement, location, PROP_LOCATION,
                          properties[PROP_LOCATION], NULL);
}

static void
grex_element_init(GrexElement *element) {
  element->children = g_ptr_array_new_with_free_func(g_object_unref);
}

/**
 * grex_element_new:
 * @root_type: The #GType this element represents.
 * @location: The source location for this element.
 *
 * Creates a new, empty #GrexElement representing the given type at the given
 * location.
 *
 * Returns: (transfer full): A new element.
 */
GrexElement *
grex_element_new(GType root_type, GrexSourceLocation *location) {
  return g_object_new(GREX_TYPE_ELEMENT, "root-type", root_type, "location",
                      location, NULL);
}

typedef struct {
  const char *filename;

  GtkBuilder *builder;
  GPtrArray *element_stack;
} GrexElementParserData;

static GrexSourceLocation *
grex_element_parser_get_location(GMarkupParseContext *context,
                                 GrexElementParserData *data) {
  int line = 0, column = 0;
  g_markup_parse_context_get_position(context, &line, &column);

  return grex_source_location_new(data->filename, line, column);
}

static void
grex_element_parser_start_element(GMarkupParseContext *context,
                                  const char *name,
                                  const char **attribute_names,
                                  const char **attribute_values,
                                  gpointer user_data, GError **error) {
  GrexElementParserData *data = user_data;
  g_autoptr(GrexSourceLocation) location =
      grex_element_parser_get_location(context, data);

  GType type = gtk_builder_get_type_from_name(data->builder, name);
  if (type == 0) {
    g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                "Unknown type: %s", name);
    return;
  }

  GrexElement *element = grex_element_new(type, g_steal_pointer(&location));
  if (data->element_stack->len > 0) {
    GrexElement *parent =
        g_ptr_array_index(data->element_stack, data->element_stack->len - 1);
    grex_element_add_child(parent, element);
  }

  g_ptr_array_add(data->element_stack, element);
}

static void
grex_element_parser_end_element(GMarkupParseContext *context, const char *name,
                                gpointer user_data, GError **error) {
  GrexElementParserData *data = user_data;

  // Make sure we don't remove the last element, so the caller can easily access
  // it.
  if (data->element_stack->len > 1) {
    g_ptr_array_steal_index(data->element_stack, data->element_stack->len - 1);
  }
}

/**
 * grex_element_parse_xml:
 * @xml: The XML content.
 * @filename: (nullable): The filename of the content, used in the resulting
 *                        element's source location.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names.
 * @error: Return location for a #GError.
 *
 * TODO
 *
 * Returns: (transfer full): A new element.
 */
GrexElement *
grex_element_parse_xml(const char *xml, const char *filename,
                       GtkBuilderScope *scope, GError **error) {
  GMarkupParser parser = {NULL};
  parser.start_element = grex_element_parser_start_element;
  parser.end_element = grex_element_parser_end_element;

  g_autoptr(GtkBuilder) builder = gtk_builder_new();
  gtk_builder_set_scope(builder, scope);

  g_autoptr(GPtrArray) element_stack =
      g_ptr_array_new_with_free_func(g_object_unref);
  GrexElementParserData data = {
      .filename = filename,
      .builder = builder,
      .element_stack = element_stack,
  };

  g_autoptr(GMarkupParseContext) context = g_markup_parse_context_new(
      &parser, G_MARKUP_PREFIX_ERROR_POSITION, &data, NULL);
  if (!g_markup_parse_context_parse(context, xml, -1, error) ||
      !g_markup_parse_context_end_parse(context, error)) {
    return NULL;
  }

  g_return_val_if_fail(element_stack->len == 1, NULL);
  return g_ptr_array_steal_index(element_stack, 0);
}

/**
 * grex_element_get_root_type:
 *
 * Returns this element's root #GType.
 *
 * Returns: The root type.
 */
GPROPZ_DEFINE_RO(GType, GrexElement, grex_element, root_type,
                 properties[PROP_ROOT_TYPE])

/**
 * grex_element_get_location:
 *
 * Returns this element's source location.
 *
 * Returns: (transfer none): The element's source location.
 */
GPROPZ_DEFINE_RO(GrexSourceLocation *, GrexElement, grex_element, location,
                 properties[PROP_LOCATION])

/**
 * grex_element_add_child:
 * @child: (transfer none): The child to add.
 *
 * Adds a new child to this element.
 */
void
grex_element_add_child(GrexElement *element, GrexElement *child) {
  g_ptr_array_add(element->children, g_object_ref(child));
}

/**
 * grex_element_get_children:
 *
 * Returns the list of children in this element.
 *
 * Returns: (element-type GrexElement) (transfer container):
 *          A list of child elements.
 */
GList *
grex_element_get_children(GrexElement *element) {
  GList *children = NULL;
  for (guint i = 0; i < element->children->len; i++) {
    children =
        g_list_prepend(children, g_ptr_array_index(element->children, i));
  }

  return g_list_reverse(children);
}
