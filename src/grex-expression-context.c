/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-expression-context.h"

#include "gpropz.h"
#include "grex-expression-context-private.h"

struct _GrexExpressionContext {
  GObject parent_instance;

  GPtrArray *scopes;
};

enum {
  SIGNAL_CHANGED = 1,
  SIGNAL_RESET,
  N_SIGNALS,
};

static guint signals[N_SIGNALS] = {0};

G_DEFINE_TYPE(GrexExpressionContext, grex_expression_context, G_TYPE_OBJECT)

static void
grex_expression_context_dispose(GObject *object) {
  GrexExpressionContext *context = GREX_EXPRESSION_CONTEXT(object);

  g_clear_pointer(&context->scopes, g_ptr_array_unref);  // NOLINT
}

static void
grex_expression_context_class_init(GrexExpressionContextClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_expression_context_dispose;

  signals[SIGNAL_CHANGED] =
      g_signal_new("changed", G_TYPE_FROM_CLASS(object_class),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  signals[SIGNAL_RESET] =
      g_signal_new("reset", G_TYPE_FROM_CLASS(object_class),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
grex_expression_context_init(GrexExpressionContext *context) {
  context->scopes = g_ptr_array_new_with_free_func(g_object_unref);
}

/**
 * grex_expression_context_new:
 *
 * Creates a new context to evaluate an expression, with an empty global scope.
 *
 * Returns: (transfer none): The new expression context.
 */
GrexExpressionContext *
grex_expression_context_new() {
  return g_object_new(GREX_TYPE_EXPRESSION_CONTEXT, NULL);
}

/**
 * grex_expression_context_add_scope:
 * @scope: (transfer none): The scope object to add.
 *
 * Adds the given object to the list of scopes in this context.
 */
void
grex_expression_context_add_scope(GrexExpressionContext *context,
                                  GObject *scope) {
  g_ptr_array_add(context->scopes, g_object_ref(scope));
  grex_expression_context_emit_changed(context);
}

/**
 * grex_expression_context_reset_dependencies:
 *
 * Resets all the expression dependencies currently emitting "changed" signals.
 */
void
grex_expression_context_reset_dependencies(GrexExpressionContext *context) {
  g_signal_emit(context, signals[SIGNAL_RESET], 0);
}

GObject *
grex_expression_context_find_object_with_property(
    GrexExpressionContext *context, const char *property) {
  for (guint i = 0; i < context->scopes->len; i++) {
    GObject *object = g_ptr_array_index(context->scopes, i);
    GObjectClass *object_class = G_OBJECT_GET_CLASS(object);
    if (g_object_class_find_property(object_class, property) != NULL) {
      return object;
    }
  }

  return NULL;
}

void
grex_expression_context_emit_changed(GrexExpressionContext *context) {
  g_object_freeze_notify(G_OBJECT(context));
  g_signal_emit(context, signals[SIGNAL_CHANGED], 0);
  g_object_thaw_notify(G_OBJECT(context));
}
