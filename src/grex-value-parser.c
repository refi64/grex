/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-value-parser.h"

typedef struct {
  GrexValueParserCallback callback;
  gpointer data;
  GDestroyNotify data_free;

  gboolean include_subtypes;
} Parser;

static void
parser_free(Parser *parser) {
  if (parser->data != NULL) {
    parser->data_free(g_steal_pointer(&parser->data));
  }

  g_free(parser);
}

struct _GrexValueParser {
  GObject parent_instance;

  GRWLock lock;
  GHashTable *parsers;
};

G_DEFINE_QUARK("grex-value-parser-error-quark", grex_value_parser_error)

G_DEFINE_FINAL_TYPE(GrexValueParser, grex_value_parser, G_TYPE_OBJECT)

static void
grex_value_parser_dispose(GObject *object) {
  GrexValueParser *parser = GREX_VALUE_PARSER(object);

  g_rw_lock_clear(&parser->lock);
  g_clear_pointer(&parser->parsers, g_hash_table_unref);
}

static void
grex_value_parser_class_init(GrexValueParserClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_value_parser_dispose;
}

static void
grex_value_parser_init(GrexValueParser *parser) {
  g_rw_lock_init(&parser->lock);
  parser->parsers = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
                                          (GDestroyNotify)parser_free);
}

static GrexValueHolder *
enum_parser(const char *string, GType type, gpointer user_data,
            GError **error) {
  GEnumClass *enum_class = G_ENUM_CLASS(g_type_class_ref(type));
  GEnumValue *enum_value = g_enum_get_value_by_nick(enum_class, string);
  if (enum_value == NULL) {
    g_set_error(error, GREX_VALUE_PARSER_ERROR,
                GREX_VALUE_PARSER_ERROR_BAD_VALUE,
                "Invalid enum value for '%s': %s",
                g_quark_to_string(g_type_qname(type)), string);
    return FALSE;
  }

  GValue value = G_VALUE_INIT;
  g_value_init(&value, type);
  g_value_set_enum(&value, enum_value->value);
  return grex_value_holder_new(&value);
}

/**
 * grex_value_parser_default:
 *
 * Retrieves the default value parser, with some default parsers pre-registered.
 *
 * Returns: (transfer none): The value parser.
 */
GrexValueParser *
grex_value_parser_default() {
  static GrexValueParser *parser = NULL;
  if (g_once_init_enter(&parser)) {
    GrexValueParser *new_parser = grex_value_parser_new();

    grex_value_parser_register(new_parser, G_TYPE_ENUM, TRUE, enum_parser, NULL,
                               NULL);

    g_once_init_leave(&parser, new_parser);
  }

  return parser;
}

/**
 * grex_value_parser_new:
 *
 * Creates a new, empty value parser.
 *
 * Returns: (transfer full): The new value parser.
 */
GrexValueParser *
grex_value_parser_new() {
  return g_object_new(GREX_TYPE_VALUE_PARSER, NULL);
}

/**
 * grex_value_parser_register:
 * @parser: The value parser.
 * @type: The type this parser returns.
 * @include_subtypes: Whether or not this parser should be responsible for
 *                    parsing subtypes of the given type.
 * @callback: (scope notified) (closure callback_data)
 *            (destroy callback_data_free) (nullable): A callback to parse new
 *            values.
 * @callback_data: (nullable): User data to pass to the callback.
 * @callback_data_free: (nullable): Destroy notify for @callback_data.
 *
 * Registers a new parsing callback with the parser, parsing values of the given
 * type.
 */
void
grex_value_parser_register(GrexValueParser *parser, GType type,
                           gboolean include_subtypes,
                           GrexValueParserCallback callback,
                           gpointer callback_data,
                           GDestroyNotify callback_data_free) {
  g_rw_lock_writer_lock(&parser->lock);

  Parser *actual_parser = g_new0(Parser, 1);
  actual_parser->callback = callback;
  actual_parser->data = callback_data;
  actual_parser->data_free = callback_data_free;
  actual_parser->include_subtypes = include_subtypes;

  g_hash_table_insert(parser->parsers, (gpointer)type, actual_parser);

  g_rw_lock_writer_unlock(&parser->lock);
}

GrexValueHolder *
grex_value_parser_try_parse(GrexValueParser *parser, const char *string,
                            GType type, GError **error) {
  g_rw_lock_reader_lock(&parser->lock);

  g_autoptr(GrexValueHolder) result = NULL;
  GType actual_type = type;

  for (;;) {
    Parser *actual_parser =
        g_hash_table_lookup(parser->parsers, (gpointer)type);
    if (actual_parser != NULL &&
        (type == actual_type || actual_parser->include_subtypes)) {
      result = actual_parser->callback(string, actual_type, actual_parser->data,
                                       error);
      break;
    }

    type = g_type_parent(type);
    if (type == 0) {
      g_set_error(error, GREX_VALUE_PARSER_ERROR,
                  GREX_VALUE_PARSER_ERROR_NO_MATCH,
                  "Type '%s' cannot be parsed: %s",
                  g_quark_to_string(g_type_qname(type)), string);
      break;
    }
  }

  g_rw_lock_reader_unlock(&parser->lock);
  return g_steal_pointer(&result);
}

GrexValueHolder *
grex_value_parser_try_transform(GrexValueParser *parser,
                                GrexValueHolder *source, GType type,
                                GError **error) {
  const GValue *source_value = grex_value_holder_get_value(source);
  if (source_value->g_type == type) {
    return grex_value_holder_ref(source);
  }

  if (g_value_type_transformable(source_value->g_type, type)) {
    g_auto(GValue) transformed_value = G_VALUE_INIT;
    g_value_init(&transformed_value, type);

    g_value_transform(source_value, &transformed_value);
    return grex_value_holder_new(&transformed_value);
  }

  if (source_value->g_type == G_TYPE_STRING) {
    return grex_value_parser_try_parse(parser, g_value_get_string(source_value),
                                       type, error);
  }

  g_set_error(error, GREX_VALUE_PARSER_ERROR, GREX_VALUE_PARSER_ERROR_NO_MATCH,
              "Type '%s' cannot be transformed from '%s'",
              g_quark_to_string(g_type_qname(type)),
              g_quark_to_string(g_type_qname(source_value->g_type)));
  return NULL;
}
