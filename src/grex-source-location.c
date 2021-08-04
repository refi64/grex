/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-source-location.h"

#include "gpropz.h"

struct _GrexSourceLocation {
  GObject parent_instance;

  char *file;
  gint line;
  gint column;
};

enum {
  PROP_FILE = 1,
  PROP_LINE,
  PROP_COLUMN,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexSourceLocation, grex_source_location, G_TYPE_OBJECT)

static void
grex_source_location_finalize(GObject *object) {
  GrexSourceLocation *loc = GREX_SOURCE_LOCATION(object);
  g_clear_pointer(&loc->file, g_free);
}

static void
grex_source_location_class_init(GrexSourceLocationClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = grex_source_location_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_FILE] =
      g_param_spec_string("file", "Source file name", "The source file name.",
                          NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexSourceLocation, file, PROP_FILE,
                          properties[PROP_FILE], NULL);

  properties[PROP_LINE] = g_param_spec_int(
      "line", "Line number", "The 1-based line number in the source file.", 0,
      G_MAXINT, 0, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexSourceLocation, line, PROP_LINE,
                          properties[PROP_LINE], NULL);

  properties[PROP_COLUMN] =
      g_param_spec_int("column", "Column number",
                       "The 1-based column number in the source file.", 0,
                       G_MAXINT, 0, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexSourceLocation, column, PROP_COLUMN,
                          properties[PROP_COLUMN], NULL);
}

static void
grex_source_location_init(GrexSourceLocation *location) {}

/**
 * grex_source_location_new:
 * @file: (nullable): The source filename.
 * @line: The 1-indexed line number in the source file.
 * @column: The 1-indexed column number in the source file.
 *
 * Creates a new #GrexSourceLocation pointing to the given line and column in
 * the given file.
 *
 * Returns: (transfer full): A new source location.
 */
GrexSourceLocation *
grex_source_location_new(const char *file, gint line, gint column) {
  return g_object_new(GREX_TYPE_SOURCE_LOCATION, "file", file, "line", line,
                      "column", column, NULL);
}

/**
 * grex_source_location_new_offset:
 * @base: The base source location.
 * @line_offset: A line offset.
 * @column_offset: A column offset.
 *
 * Creates a new #GrexSourceLocation offset from the @base location by the given
 * number of lines and columns.
 *
 * Returns: (transfer full): A new source location.
 */
GrexSourceLocation *
grex_source_location_new_offset(GrexSourceLocation *base, gint line_offset,
                                gint column_offset) {
  gint base_line = grex_source_location_get_line(base);
  gint base_column = grex_source_location_get_column(base);

  return grex_source_location_new(grex_source_location_get_file(base),
                                  base_line != 0 ? base_line + line_offset : 0,
                                  base_column != 0 ? base_column + column_offset
                                                   : 0);
}

/**
 * grex_source_location_get_file:
 *
 * Retrieves the source location's file name.
 *
 * Returns: (transfer none): The file name.
 */
GPROPZ_DEFINE_RO(const char *, GrexSourceLocation, grex_source_location, file,
                 properties[PROP_FILE])

/**
 * grex_source_location_get_line:
 *
 * Retrieves the source location's line number.
 *
 * Returns: The line number.
 */
GPROPZ_DEFINE_RO(gint, GrexSourceLocation, grex_source_location, line,
                 properties[PROP_LINE])

/**
 * grex_source_location_get_column:
 *
 * Retrieves the source location's column number.
 *
 * Returns: The column number.
 */
GPROPZ_DEFINE_RO(gint, GrexSourceLocation, grex_source_location, column,
                 properties[PROP_COLUMN])

/**
 * grex_source_location_format:
 *
 * Formats this source location as a string in the format "file:line:column".
 *
 * Returns: (transfer full): The formatted location.
 */
char *
grex_source_location_format(GrexSourceLocation *loc) {
  g_autofree char *line =
      loc->line != 0 ? g_strdup_printf("%d", loc->line) : NULL;
  g_autofree char *column =
      loc->column != 0 ? g_strdup_printf("%d", loc->column) : NULL;

  return g_strdup_printf(
      "%s:%s:%s", loc->file != NULL ? loc->file : "<unknown>",
      line != NULL ? line : "?", column != NULL ? column : "?");
}
