/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-binding.h"

#include "gpropz.h"

typedef enum { SEGMENT_CONSTANT } SegmentType;

typedef struct {
  SegmentType type;
  union {
    char *constant;
  };
} Segment;

struct _GrexBinding {
  GObject parent_instance;

  char *target;
  GrexSourceLocation *location;

  GPtrArray *segments;
};

enum {
  PROP_TARGET = 1,
  PROP_LOCATION,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexBinding, grex_binding, G_TYPE_OBJECT)

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

  g_clear_object(&binding->location);  // NOLINT
}

static void
grex_binding_finalize(GObject *object) {
  GrexBinding *binding = GREX_BINDING(object);

  g_clear_pointer(&binding->target, g_free);
}

static void
grex_binding_class_init(GrexBindingClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_binding_dispose;
  object_class->finalize = grex_binding_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_TARGET] =
      g_param_spec_string("target", "Target property name",
                          "The name of the property this binding is targeting.",
                          NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexBinding, target, PROP_TARGET,
                          properties[PROP_TARGET], NULL);

  properties[PROP_LOCATION] = g_param_spec_object(
      "location", "Source location",
      "The source location this binding is from.", GREX_TYPE_SOURCE_LOCATION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexBinding, location, PROP_LOCATION,
                          properties[PROP_LOCATION], NULL);
}

static void
grex_binding_init(GrexBinding *binding) {
  binding->segments =
      g_ptr_array_new_with_free_func((GDestroyNotify)segment_free);
}

/**
 * grex_binding_new:
 * @target: The name of the attribute this binding targets.
 * @location: The source location for this binding.
 *
 * Creates a new, empty #GrexBinding targeting the given target attribute and at
 * the given source location.
 *
 * Returns: (transfer full): A new binding.
 */
GrexBinding *
grex_binding_new(const char *target, GrexSourceLocation *location) {
  return g_object_new(GREX_TYPE_BINDING, "target", target, "location", location,
                      NULL);
}

/**
 * grex_binding_get_target:
 *
 * Returns this binding's target attribute name.
 *
 * Returns: (transfer none): The binding's target.
 */
GPROPZ_DEFINE_RO(const char *, GrexBinding, grex_binding, target,
                 properties[PROP_TARGET])

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
 * grex_binding_add_constant:
 * @content: The constant content to add.
 *
 * Appends a constant string to this binding.
 */
void
grex_binding_add_constant(GrexBinding *binding, const char *content) {
  Segment *segment = g_new0(Segment, 1);
  segment->type = SEGMENT_CONSTANT;
  segment->constant = g_strdup(content);

  g_ptr_array_add(binding->segments, segment);
}

/**
 * grex_binding_evaluate:
 * @error: Return location for a #GError.
 *
 * Evaluates this binding and returns the result.
 *
 * Returns: (transfer full): The evaluated string.
 */
char *
grex_binding_evaluate(GrexBinding *binding, GError **error) {
  g_autoptr(GString) result = g_string_new("");

  for (guint i = 0; i < binding->segments->len; i++) {
    Segment *segment = g_ptr_array_index(binding->segments, i);
    switch (segment->type) {
    case SEGMENT_CONSTANT:
      g_string_append(result, segment->constant);
      break;
    }
  }

  return g_string_free(g_steal_pointer(&result), FALSE);
}
