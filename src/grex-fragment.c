/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-fragment.h"

#include "gpropz.h"

/*
 * GrexFragment:
 *
 * TODO: docs
 */
struct _GrexFragment {
  GObject parent_instance;

  GType root_type;
  GrexSourceLocation *location;

  GPtrArray *children;
};

typedef struct {
  GrexSourceLocation *location;
} GrexFragmentParseErrorPrivate;

enum {
  PROP_ROOT_TYPE = 1,
  PROP_LOCATION,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexFragment, grex_fragment, G_TYPE_OBJECT)

static void
grex_fragment_dispose(GObject *object) {
  GrexFragment *fragment = GREX_FRAGMENT(object);

  g_clear_object(&fragment->location);                      // NOLINT
  g_clear_pointer(&fragment->children, g_ptr_array_unref);  // NOLINT
}

static void
grex_fragment_class_init(GrexFragmentClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_fragment_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_ROOT_TYPE] = g_param_spec_gtype(
      "root-type", "Root type", "The type this fragment represents.",
      G_TYPE_NONE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragment, root_type, PROP_ROOT_TYPE,
                          properties[PROP_ROOT_TYPE], NULL);

  properties[PROP_LOCATION] = g_param_spec_object(
      "location", "Source location",
      "The source location this fragment is from.", GREX_TYPE_SOURCE_LOCATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragment, location, PROP_LOCATION,
                          properties[PROP_LOCATION], NULL);
}

static void
grex_fragment_init(GrexFragment *fragment) {
  fragment->children = g_ptr_array_new_with_free_func(g_object_unref);
}

/**
 * grex_fragment_new:
 * @root_type: The #GType this fragment represents.
 * @location: The source location for this fragment.
 *
 * Creates a new, empty #GrexFragment representing the given type at the given
 * location.
 *
 * Returns: (transfer full): A new fragment.
 */
GrexFragment *
grex_fragment_new(GType root_type, GrexSourceLocation *location) {
  return g_object_new(GREX_TYPE_FRAGMENT, "root-type", root_type, "location",
                      location, NULL);
}

typedef struct {
  const char *filename;

  GtkBuilder *builder;
  GPtrArray *fragment_stack;
} GrexFragmentParserData;

static GrexSourceLocation *
grex_fragment_parser_get_location(GMarkupParseContext *context,
                                  GrexFragmentParserData *data) {
  int line = 0, column = 0;
  g_markup_parse_context_get_position(context, &line, &column);

  return grex_source_location_new(data->filename, line, column);
}

static void
grex_fragment_parser_start_fragment(GMarkupParseContext *context,
                                    const char *name,
                                    const char **attribute_names,
                                    const char **attribute_values,
                                    gpointer user_data, GError **error) {
  GrexFragmentParserData *data = user_data;
  g_autoptr(GrexSourceLocation) location =
      grex_fragment_parser_get_location(context, data);

  GType type = gtk_builder_get_type_from_name(data->builder, name);
  if (type == 0) {
    g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                "Unknown type: %s", name);
    return;
  }

  GrexFragment *fragment = grex_fragment_new(type, g_steal_pointer(&location));
  if (data->fragment_stack->len > 0) {
    GrexFragment *parent =
        g_ptr_array_index(data->fragment_stack, data->fragment_stack->len - 1);
    grex_fragment_add_child(parent, fragment);
  }

  g_ptr_array_add(data->fragment_stack, fragment);
}

static void
grex_fragment_parser_end_fragment(GMarkupParseContext *context,
                                  const char *name, gpointer user_data,
                                  GError **error) {
  GrexFragmentParserData *data = user_data;

  // Make sure we don't remove the last fragment, so the caller can easily
  // access it.
  if (data->fragment_stack->len > 1) {
    g_ptr_array_steal_index(data->fragment_stack,
                            data->fragment_stack->len - 1);
  }
}

/**
 * grex_fragment_parse_xml:
 * @xml: The XML content.
 * @filename: (nullable): The filename of the content, used in the resulting
 *                        fragment's source location.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names.
 * @error: Return location for a #GError.
 *
 * TODO
 *
 * Returns: (transfer full): A new fragment.
 */
GrexFragment *
grex_fragment_parse_xml(const char *xml, const char *filename,
                        GtkBuilderScope *scope, GError **error) {
  GMarkupParser parser = {NULL};
  parser.start_element = grex_fragment_parser_start_fragment;
  parser.end_element = grex_fragment_parser_end_fragment;

  g_autoptr(GtkBuilder) builder = gtk_builder_new();
  gtk_builder_set_scope(builder, scope);

  g_autoptr(GPtrArray) fragment_stack =
      g_ptr_array_new_with_free_func(g_object_unref);
  GrexFragmentParserData data = {
      .filename = filename,
      .builder = builder,
      .fragment_stack = fragment_stack,
  };

  g_autoptr(GMarkupParseContext) context = g_markup_parse_context_new(
      &parser, G_MARKUP_PREFIX_ERROR_POSITION, &data, NULL);
  if (!g_markup_parse_context_parse(context, xml, -1, error) ||
      !g_markup_parse_context_end_parse(context, error)) {
    return NULL;
  }

  g_return_val_if_fail(fragment_stack->len == 1, NULL);
  return g_ptr_array_steal_index(fragment_stack, 0);
}

/**
 * grex_fragment_get_root_type:
 *
 * Returns this fragment's root #GType.
 *
 * Returns: The root type.
 */
GPROPZ_DEFINE_RO(GType, GrexFragment, grex_fragment, root_type,
                 properties[PROP_ROOT_TYPE])

/**
 * grex_fragment_get_location:
 *
 * Returns this fragment's source location.
 *
 * Returns: (transfer none): The fragment's source location.
 */
GPROPZ_DEFINE_RO(GrexSourceLocation *, GrexFragment, grex_fragment, location,
                 properties[PROP_LOCATION])

/**
 * grex_fragment_add_child:
 * @child: (transfer none): The child to add.
 *
 * Adds a new child to this fragment.
 */
void
grex_fragment_add_child(GrexFragment *fragment, GrexFragment *child) {
  g_ptr_array_add(fragment->children, g_object_ref(child));
}

/**
 * grex_fragment_get_children:
 *
 * Returns the list of children in this fragment.
 *
 * Returns: (element-type GrexFragment) (transfer container):
 *          A list of child fragments.
 */
GList *
grex_fragment_get_children(GrexFragment *fragment) {
  GList *children = NULL;
  for (guint i = 0; i < fragment->children->len; i++) {
    children =
        g_list_prepend(children, g_ptr_array_index(fragment->children, i));
  }

  return g_list_reverse(children);
}
