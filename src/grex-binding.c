/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-binding.h"

#include "gpropz.h"
#include "grex-enums.h"
#include "grex-parser-private.h"
#include "grex-value-parser.h"

typedef enum {
  SEGMENT_CONSTANT,
  SEGMENT_EXPRESSION,
} SegmentType;

typedef struct {
  SegmentType type;
  union {
    char *constant;
    struct {
      GrexExpression *expression;
      gboolean is_bidirectional;
    };
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

G_DEFINE_QUARK("grex-binding-parse-error-quark", grex_binding_parse_error)
G_DEFINE_QUARK("grex-binding-evaluation-error-quark",
               grex_binding_evaluation_error)

static void
segment_free(Segment *segment) {
  switch (segment->type) {
  case SEGMENT_CONSTANT:
    g_clear_pointer(&segment->constant, g_free);
    break;
  case SEGMENT_EXPRESSION:
    g_clear_object(&segment->expression);  // NOLINT
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
grex_binding_evaluate(GrexBinding *binding, GType expected_type,
                      GrexExpressionContext *eval_context,
                      gboolean track_dependencies, GError **error) {
  if (G_UNLIKELY(binding->segments == NULL)) {
    g_warning(
        "GrexBinding instances may only be created via GrexBindingBuilder");
    return FALSE;
  }

  g_autoptr(GrexValueHolder) result = NULL;

  GrexExpressionEvaluationFlags flags = 0;
  if (track_dependencies) {
    flags |= GREX_EXPRESSION_EVALUATION_TRACK_DEPENDENCIES;
  }

  gboolean requires_push = binding->type == GREX_BINDING_TYPE_EXPRESSION_2WAY;

  if (grex_binding_type_is_expression(binding->type)) {
    if (requires_push) {
      flags |= GREX_EXPRESSION_EVALUATION_ENABLE_PUSH;
    }

    g_return_val_if_fail(binding->segments->len == 1, NULL);
    Segment *first_segment = g_ptr_array_index(binding->segments, 0);

    result = grex_expression_evaluate(first_segment->expression, eval_context,
                                      flags, error);
    if (result == NULL) {
      return NULL;
    }
  } else {
    g_autoptr(GString) result_string = g_string_new("");

    for (guint i = 0; i < binding->segments->len; i++) {
      Segment *segment = g_ptr_array_index(binding->segments, i);

      switch (segment->type) {
      case SEGMENT_CONSTANT:
        g_string_append(result_string, segment->constant);
        break;
      case SEGMENT_EXPRESSION: {
        g_autoptr(GrexValueHolder) value_holder = grex_expression_evaluate(
            segment->expression, eval_context, flags, error);
        if (value_holder == NULL) {
          return NULL;
        }

        const GValue *value = grex_value_holder_get_value(value_holder);
        GType current_type = G_VALUE_TYPE(value);
        if (current_type != G_TYPE_STRING) {
          if (!g_value_type_transformable(current_type, G_TYPE_STRING)) {
            grex_set_located_error(
                error, binding->location, GREX_BINDING_EVALUATION_ERROR,
                GREX_BINDING_EVALUATION_ERROR_INVALID_TYPE,
                "Expression in compound binding must be transformable to a "
                "string, but type '%s' is not",
                g_type_name(current_type));
            return NULL;
          }

          g_auto(GValue) transformed_value = G_VALUE_INIT;
          g_value_init(&transformed_value, G_TYPE_STRING);
          g_value_transform(value, &transformed_value);

          g_string_append(result_string,
                          g_value_get_string(&transformed_value));
        } else {
          g_string_append(result_string, g_value_get_string(value));
        }

        break;
      }
      }
    }

    g_auto(GValue) value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_value_take_string(&value,
                        g_string_free(g_steal_pointer(&result_string), FALSE));
    result = grex_value_holder_new(&value);
  }

  gboolean lost_push_during_transform = FALSE;
  if (expected_type != G_TYPE_NONE &&
      G_VALUE_TYPE(grex_value_holder_get_value(result)) != expected_type) {
    g_autoptr(GError) local_error = NULL;
    g_autoptr(GrexValueHolder) transformed_value =
        grex_value_parser_try_transform(grex_value_parser_default(), result,
                                        expected_type, &local_error);
    if (transformed_value == NULL) {
      grex_set_located_error(error, binding->location, local_error->domain,
                             local_error->code, "%s", local_error->message);
      return NULL;
    }

    if (grex_value_holder_can_push(result) &&
        !grex_value_holder_can_push(transformed_value)) {
      lost_push_during_transform = TRUE;
    }

    g_clear_pointer(&result, grex_value_holder_unref);  // NOLINT
    result = g_steal_pointer(&transformed_value);
  }

  if (requires_push) {
    if (!grex_value_holder_can_push(result)) {
      grex_set_located_error(
          error, binding->location, GREX_BINDING_EVALUATION_ERROR,
          GREX_BINDING_EVALUATION_ERROR_NON_BIDIRECTIONAL,
          "Binding result must be bidirectional%s",
          lost_push_during_transform
              ? " (bidirectionality was lost during transform)"
              : "");
      return NULL;
    }
  } else {
    grex_value_holder_disable_push(result);
  }

  return g_steal_pointer(&result);
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
 * @len: Length of @content in bytes, or -1 if null-terminated.
 *
 * Appends a constant string to this builder.
 */
void
grex_binding_builder_add_constant(GrexBindingBuilder *builder,
                                  const char *content, gssize len) {
  g_return_if_fail(grex_binding_builder_check_not_built(builder));

  Segment *segment = g_new0(Segment, 1);
  segment->type = SEGMENT_CONSTANT;
  segment->constant = len != -1 ? g_strndup(content, len) : g_strdup(content);

  g_ptr_array_add(builder->segments, segment);
}

void
grex_binding_builder_add_expression(GrexBindingBuilder *builder,
                                    GrexExpression *expression,
                                    gboolean is_bidirectional) {
  g_return_if_fail(grex_binding_builder_check_not_built(builder));

  Segment *segment = g_new0(Segment, 1);
  segment->type = SEGMENT_EXPRESSION;
  segment->expression = g_object_ref(expression);
  segment->is_bidirectional = is_bidirectional;

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

      // Multiple constants in a row can still be treated as a constant result.
      for (guint i = 1; i < builder->segments->len; i++) {
        Segment *segment = g_ptr_array_index(builder->segments, i);
        if (segment->type != first_segment->type) {
          type = GREX_BINDING_TYPE_COMPOUND;
          break;
        }
      }
      break;
    case SEGMENT_EXPRESSION:
      if (builder->segments->len > 1) {
        // Unlike constants, multiple exprs in a row CANNOT be treated as
        // non-compound.
        type = GREX_BINDING_TYPE_COMPOUND;
      } else {
        type = first_segment->is_bidirectional
                   ? GREX_BINDING_TYPE_EXPRESSION_2WAY
                   : GREX_BINDING_TYPE_EXPRESSION_1WAY;
      }
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

/**
 * grex_binding_parse:
 * @content: The binding source string.
 * @location: The binding's source location.
 * @error: Return location for a #GError.
 *
 * Parses the given binding string.
 *
 * Returns: (transfer full): The parsed binding, or NULL on error.
 */
GrexBinding *
grex_binding_parse(const char *content, GrexSourceLocation *location,
                   GError **error) {
  g_autoptr(GrexBindingBuilder) builder = grex_binding_builder_new();

  int line_offset = 0, col_offset = 0;

  while (*content != '\0') {
    const char *expr_start = strpbrk(content, "{[");
    if (expr_start != content) {
      // Add the constant text in between, then quit out if this was the end.
      gboolean is_end = expr_start == NULL;
      grex_binding_builder_add_constant(builder, content,
                                        is_end ? -1 : expr_start - content);
      if (is_end) {
        break;
      }
    }

    gboolean is_bidirectional = *expr_start == '{';
    char closing_bracket = is_bidirectional ? '}' : ']';

    // Move past the opening bracket, so the expression's location info will
    // point past the bracket.
    expr_start++;

    update_location(content, expr_start, &line_offset, &col_offset);
    g_autoptr(GrexSourceLocation) expr_location =
        grex_source_location_new_offset(location, line_offset, col_offset);

    const char *expr_end = strchr(expr_start, closing_bracket);
    if (expr_end == NULL) {
      grex_set_located_error(error, expr_location, GREX_BINDING_PARSE_ERROR,
                             GREX_BINDING_PARSE_ERROR_MISMATCHED_BRACKET,
                             "Missing closing bracket '%c'", closing_bracket);
      return NULL;
    }

    g_autoptr(GrexExpression) expression = grex_expression_parse(
        expr_start, expr_end - expr_start, expr_location, error);
    if (expression == NULL) {
      return NULL;
    }

    grex_binding_builder_add_expression(builder, expression, is_bidirectional);

    content = expr_end + 1;
    update_location(expr_start, content, &line_offset, &col_offset);
  }

  return grex_binding_builder_build(builder, location);
}
