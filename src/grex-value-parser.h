/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"
#include "grex-value-holder.h"

G_BEGIN_DECLS

typedef enum {
  GREX_VALUE_PARSER_ERROR_NO_MATCH,
  GREX_VALUE_PARSER_ERROR_BAD_VALUE,
} GrexValueParserError;

#define GREX_VALUE_PARSER_ERROR grex_value_parser_error_quark()
GQuark grex_value_parser_error_quark();

#define GREX_TYPE_VALUE_PARSER grex_value_parser_get_type()
G_DECLARE_FINAL_TYPE(GrexValueParser, grex_value_parser, GREX, VALUE_PARSER,
                     GObject)

typedef GrexValueHolder *(*GrexValueParserCallback)(const char *string,
                                                    GType type,
                                                    gpointer user_data,
                                                    GError **error);

GrexValueParser *grex_value_parser_default();

GrexValueParser *grex_value_parser_new();

void grex_value_parser_register(GrexValueParser *parser, GType type,
                                gboolean include_subtypes,
                                GrexValueParserCallback callback,
                                gpointer callback_data,
                                GDestroyNotify callback_data_free);

GrexValueHolder *grex_value_parser_try_parse(GrexValueParser *parser,
                                             const char *string, GType type,
                                             GError **error);

GrexValueHolder *grex_value_parser_try_transform(GrexValueParser *parser,
                                                 GrexValueHolder *source,
                                                 GType type, GError **error);

G_END_DECLS
