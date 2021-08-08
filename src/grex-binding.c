/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-binding.h"

#include "gpropz.h"
#include "grex-enums.h"

typedef enum { SEGMENT_CONSTANT } SegmentType;

typedef struct {
  SegmentType type;
  union {
    char *constant;
  };
} Segment;

struct _GrexBinding {
  GObject parent_instance;

  GrexBindingType type;
  GrexSourceLocation *location;

  GPtrArray *segments;
};

struct _GrexBindingBuilder {
  GObject parent_instance;

  GPtrArray *segments;
};

enum {
  PROP_BINDING_TYPE = 1,
  PROP_LOCATION,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexBinding, grex_binding, G_TYPE_OBJECT)
G_DEFINE_TYPE(GrexBindingBuilder, grex_binding_builder, G_TYPE_OBJECT)

static void
segment_free(Segment *segment) {
  switch (segment->type) {
  case SEGMENT_CONSTANT:
    g_clear_pointer(&segment->constant, g_free);
    break;
  }
}

static void
grex_binding_dispose(GObject *object) {
  GrexBinding *binding = GREX_BINDING(object);

  g_clear_object(&binding->location);                      // NOLINT
  g_clear_pointer(&binding->segments, g_ptr_array_unref);  // NOLINT
}

static void
grex_binding_class_init(GrexBindingClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_binding_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_BINDING_TYPE] = g_param_spec_enum(
      "binding-type", "Binding type", "The type of this Grex binding.",
      GREX_TYPE_BINDING_TYPE, GREX_BINDING_TYPE_CONSTANT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexBinding, type, PROP_BINDING_TYPE,
                          properties[PROP_BINDING_TYPE], NULL);

  properties[PROP_LOCATION] = g_param_spec_object(
      "location", "Source location",
      "The source location this binding is from.", GREX_TYPE_SOURCE_LOCATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexBinding, location, PROP_LOCATION,
                          properties[PROP_LOCATION], NULL);
}

static void
grex_binding_init(GrexBinding *binding) {}

/**
 * grex_binding_get_binding_type:
 *
 * Returns this binding's type.
 *
 * Returns: The binding type.
 */
GPROPZ_DEFINE_RO(GrexBindingType, GrexBinding, grex_binding, binding_type,
                 properties[PROP_BINDING_TYPE])

/**
 * grex_binding_get_location:
 *
 * Returns this binding's source location.
 *
 * Returns: (transfer none): The binding's source location.
 */
GPROPZ_DEFINE_RO(GrexSourceLocation *, GrexBinding, grex_binding, location,
                 properties[PROP_LOCATION])

/**
 * grex_binding_evaluate:
 * @error: Return location for a #GError.
 *
 * Evaluates this binding and returns the resulting value.
 *
 * Returns: The resulting value, or NULL on error.
 */
GrexValueHolder *
grex_binding_evaluate(GrexBinding *binding, GError **error) {
  if (G_UNLIKELY(binding->segments == NULL)) {
    g_warning(
        "GrexBinding instances may only be created via GrexBindingBuilder");
    return FALSE;
  }

  if (binding->type != GREX_BINDING_TYPE_CONSTANT) {
    g_warning("Not implemented");
    return FALSE;
  }

  g_autoptr(GString) result = g_string_new("");

  for (guint i = 0; i < binding->segments->len; i++) {
    Segment *segment = g_ptr_array_index(binding->segments, i);
    switch (segment->type) {
    case SEGMENT_CONSTANT:
      g_string_append(result, segment->constant);
      break;
    }
  }

  g_auto(GValue) value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_STRING);
  g_value_take_string(&value, g_string_free(g_steal_pointer(&result), FALSE));
  return grex_value_holder_new(&value);
}

static void
grex_binding_builder_dispose(GObject *object) {
  GrexBindingBuilder *builder = GREX_BINDING_BUILDER(object);

  g_clear_pointer(&builder->segments, g_ptr_array_unref);  // NOLINT
}

static void
grex_binding_builder_class_init(GrexBindingBuilderClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_binding_builder_dispose;
}

static void
grex_binding_builder_init(GrexBindingBuilder *builder) {
  builder->segments =
      g_ptr_array_new_with_free_func((GDestroyNotify)segment_free);
}

GrexBindingBuilder *
grex_binding_builder_new() {
  return g_object_new(GREX_TYPE_BINDING_BUILDER, NULL);
}

static gboolean
grex_binding_builder_check_not_built(GrexBindingBuilder *builder) {
  if (G_UNLIKELY(builder->segments == NULL)) {
    g_warning("Builder can only have _build called once");
    return FALSE;
  }

  return TRUE;
}

/**
 * grex_binding_builder_add_constant:
 * @content: The constant content to add.
 *
 * Appends a constant string to this builder.
 */
void
grex_binding_builder_add_constant(GrexBindingBuilder *builder,
                                  const char *content) {
  g_return_if_fail(grex_binding_builder_check_not_built(builder));

  Segment *segment = g_new0(Segment, 1);
  segment->type = SEGMENT_CONSTANT;
  segment->constant = g_strdup(content);

  g_ptr_array_add(builder->segments, segment);
}

/**
 * grex_binding_builder_build:
 * @location: The source location to associate with the binding.
 *
 * Returns: (transfer full): A new #GrexBinding from this builder. After calling
 *          this, the builder should be treated as "destroyed" and cannot be
 *          used.
 */
GrexBinding *
grex_binding_builder_build(GrexBindingBuilder *builder,
                           GrexSourceLocation *location) {
  g_return_val_if_fail(grex_binding_builder_check_not_built(builder), NULL);

  GrexBindingType type;

  if (builder->segments->len == 0) {
    type = GREX_BINDING_TYPE_CONSTANT;
  } else {
    Segment *first_segment = g_ptr_array_index(builder->segments, 0);
    switch (first_segment->type) {
    case SEGMENT_CONSTANT:
      type = GREX_BINDING_TYPE_CONSTANT;
      break;
    default:
      g_warning("Unexpected segment type '%d'", first_segment->type);
      return NULL;
    }

    for (guint i = 1; i < builder->segments->len; i++) {
      Segment *segment = g_ptr_array_index(builder->segments, i);
      if (segment->type != first_segment->type) {
        type = GREX_BINDING_TYPE_COMPOUND;
        break;
      }
    }
  }

  GrexBinding *binding = g_object_new(GREX_TYPE_BINDING, "binding-type", type,
                                      "location", location, NULL);
  binding->segments = g_steal_pointer(&builder->segments);
  return binding;
}
