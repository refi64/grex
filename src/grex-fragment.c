/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-fragment.h"

#include "gpropz.h"
#include "grex-binding.h"

/*
 * GrexFragment:
 *
 * TODO: docs
 */
struct _GrexFragment {
  GObject parent_instance;

  GType target_type;
  GrexSourceLocation *location;

  GHashTable *bindings;
  GPtrArray *children;
};

enum {
  PROP_target_type = 1,
  PROP_LOCATION,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexFragment, grex_fragment, G_TYPE_OBJECT)

static void
grex_fragment_dispose(GObject *object) {
  GrexFragment *fragment = GREX_FRAGMENT(object);

  g_clear_object(&fragment->location);                       // NOLINT
  g_clear_pointer(&fragment->bindings, g_hash_table_unref);  // NOLINT
  g_clear_pointer(&fragment->children, g_ptr_array_unref);   // NOLINT
}

static void
grex_fragment_class_init(GrexFragmentClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_fragment_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_target_type] = g_param_spec_gtype(
      "target-type", "Target type", "The type this fragment represents.",
      G_TYPE_NONE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragment, target_type,
                          PROP_target_type, properties[PROP_target_type], NULL);

  properties[PROP_LOCATION] = g_param_spec_object(
      "location", "Source location",
      "The source location this fragment is from.", GREX_TYPE_SOURCE_LOCATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexFragment, location, PROP_LOCATION,
                          properties[PROP_LOCATION], NULL);
}

static void
grex_fragment_init(GrexFragment *fragment) {
  fragment->bindings =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
  fragment->children = g_ptr_array_new_with_free_func(g_object_unref);
}

/**
 * grex_fragment_new:
 * @target_type: The #GType this fragment represents.
 * @location: The source location for this fragment.
 *
 * Creates a new, empty #GrexFragment representing the given type at the given
 * location.
 *
 * Returns: (transfer full): A new fragment.
 */
GrexFragment *
grex_fragment_new(GType target_type, GrexSourceLocation *location) {
  return g_object_new(GREX_TYPE_FRAGMENT, "target-type", target_type,
                      "location", location, NULL);
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

  for (; *attribute_names != NULL && *attribute_values != NULL;
       attribute_names++, attribute_values++) {
    const char *name = *attribute_names;
    const char *value = *attribute_values;

    // GMarkupParser doesn't give us exact attribute location details, so we
    // just lie about the filename to avoid giving misleading column #s.
    g_autofree char *binding_location_name = g_strdup_printf("<%s>", name);
    g_autoptr(GrexSourceLocation) binding_location =
        grex_source_location_new(binding_location_name, 1, 1);

    g_autoptr(GrexBinding) binding =
        grex_binding_parse(value, binding_location, error);
    if (binding == NULL) {
      grex_prefix_error_with_location(error, location);
      return;
    }

    grex_fragment_insert_binding(fragment, name, binding);
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
 * @len: Length of @xml in bytes, or -1 if null-terminated.
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
grex_fragment_parse_xml(const char *xml, gssize len, const char *filename,
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
  if (!g_markup_parse_context_parse(context, xml, len, error) ||
      !g_markup_parse_context_end_parse(context, error)) {
    return NULL;
  }

  g_return_val_if_fail(fragment_stack->len == 1, NULL);
  return g_ptr_array_steal_index(fragment_stack, 0);
}

/**
 * grex_fragment_get_target_type:
 *
 * Returns this fragment's target #GType.
 *
 * Returns: The target type.
 */
GPROPZ_DEFINE_RO(GType, GrexFragment, grex_fragment, target_type,
                 properties[PROP_target_type])

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
 * grex_fragment_insert_binding:
 * @target: The binding's target property.
 * @binding: (transfer none): The binding to insert.
 *
 * Inserts a new property binding into this fragment, overwriting any existing
 * bindings.
 */
void
grex_fragment_insert_binding(GrexFragment *fragment, const char *target,
                             GrexBinding *binding) {
  g_hash_table_insert(fragment->bindings, g_strdup(target),
                      g_object_ref(binding));
}

/**
 * grex_fragment_get_binding_targets:
 *
 * Returns the names of the targets contained within this fragment's property
 * bindings under the same name.
 *
 * Returns: (element-type utf8) (transfer container): A list of target names.
 */
GList *
grex_fragment_get_binding_targets(GrexFragment *fragment) {
  return g_hash_table_get_keys(fragment->bindings);
}

/**
 * grex_fragment_get_binding:
 *
 * Returns the binding associated with the given target name.
 *
 * Returns: (transfer none): The binding associated with the given target name,
 *          or NULL if it could not be found.
 */
GrexBinding *
grex_fragment_get_binding(GrexFragment *fragment, const char *target) {
  return g_hash_table_lookup(fragment->bindings, target);
}

/**
 * grex_fragment_remove_binding:
 * @target: The binding target to remove.
 *
 * Removes the binding from this fragment.
 *
 * Returns: TRUE if the binding was present.
 */
gboolean
grex_fragment_remove_binding(GrexFragment *fragment, const char *target) {
  return g_hash_table_remove(fragment->bindings, target);
}

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
