/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-expression-private.h"
#include "grex-source-location.h"

#include <glib.h>

// Declare g_autoptr cleanu pfuncs for packcc's context type.
typedef struct grex_parser_impl_context_tag grex_parser_impl_context_t;
void grex_parser_impl_destroy(grex_parser_impl_context_t *ctx);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(grex_parser_impl_context_t,
                              grex_parser_impl_destroy)

typedef enum {
  GREX_PARSER_EXPRESSION = 'E',
} GrexParserType;

typedef union {
  GrexExpression *expr;
} GrexParserResult;

typedef struct {
  GrexSourceLocation *location;
  const char *str;
  size_t len;
  gssize pos;

  GrexParserType type;
  gboolean sent_type;

  GError **error;
} Auxil;

G_GNUC_UNUSED static inline void
auxil_free(Auxil *auxil) {
  g_free(auxil);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Auxil, auxil_free)

G_GNUC_UNUSED static inline Auxil *
auxil_create(GrexSourceLocation *location, GrexParserType type, const char *str,
             gssize len, GError **error) {
  Auxil *auxil = g_new0(Auxil, 1);
  auxil->location = location;
  auxil->str = str;
  auxil->len = len != -1 ? len : strlen(str);
  auxil->type = type;
  auxil->error = error;
  return auxil;
}

G_GNUC_UNUSED static inline gboolean
auxil_is_eof(Auxil *auxil) {
  return auxil->pos >= auxil->len;
}

G_GNUC_UNUSED static inline int
auxil_nextchar(Auxil *auxil) {
  if (!auxil->sent_type) {
    auxil->sent_type = TRUE;
    return auxil->type;
  }

  if (auxil_is_eof(auxil)) {
    return -1;
  }

  return auxil->str[auxil->pos++];
}

G_GNUC_UNUSED static void
update_location(const char *start, const char *end, int *line, int *col) {
  for (;;) {
    const char *newline = memchr(start, '\n', end - start);
    if (newline == NULL) {
      break;
    }

    (*line)++;
    *col = 0;
    start = newline + 1;
  }

  *col += end - start;
}

G_GNUC_UNUSED static GrexSourceLocation *
auxil_get_location(Auxil *auxil, size_t pos) {
  int line = 0, col = 0;
  update_location(auxil->str, auxil->str + pos, &line, &col);
  return grex_source_location_new_offset(auxil->location, line, col);
}

#define OWN_OBJ(x)                 \
  G_GNUC_UNUSED g_autoptr(GObject) \
  G_PASTE(v, __LINE__) = G_OBJECT(x)
#define OWN_STR(x) G_GNUC_UNUSED g_autofree char *G_PASTE(v, __LINE__) = x
#define AUXIL_GET_LOCATION(pos) \
  g_autoptr(GrexSourceLocation) location = auxil_get_location(auxil, pos)

G_GNUC_UNUSED static void
auxil_error(Auxil *auxil) {
  g_return_if_fail(auxil->error == NULL);

  AUXIL_GET_LOCATION(auxil->pos);
  grex_set_expression_parse_error(auxil->error, location, 0, "Invalid syntax");
}

G_GNUC_UNUSED static void
auxil_expected_eof(Auxil *auxil) {
  g_warn_if_fail(auxil_is_eof(auxil));

  AUXIL_GET_LOCATION(auxil->pos);
  grex_set_expression_parse_error(auxil->error, location, 0, "Expected EOF");
}

G_GNUC_UNUSED static char *
process_string_escapes(const char *s) {
  g_autoptr(GString) result = g_string_new("");
  for (; *s != '\0'; s++) {
    if (*s == '\\') {
      switch (*++s) {
      case 'n':
        g_string_append_c(result, '\n');
        break;
      case 't':
        g_string_append_c(result, '\t');
        break;
      default:
        g_string_append_c(result, *s);
        continue;
      }
    } else {
      g_string_append_c(result, *s);
    }
  }

  return g_string_free(g_steal_pointer(&result), FALSE);
}
