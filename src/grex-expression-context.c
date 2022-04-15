/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-expression-context.h"

#include "gpropz.h"
#include "grex-expression-context-private.h"

struct _GrexExpressionContext {
  GObject parent_instance;

  GObject *scope;
  GHashTable *extra_names;
};

enum {
  PROP_SCOPE = 1,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

enum {
  SIGNAL_CHANGED = 1,
  SIGNAL_RESET,
  N_SIGNALS,
};

static guint signals[N_SIGNALS] = {0};

G_DEFINE_TYPE(GrexExpressionContext, grex_expression_context, G_TYPE_OBJECT)

static void
destroy_gvalue(GValue *value) {
  g_value_unset(value);
  g_free(value);
}

static void
grex_expression_context_dispose(GObject *object) {
  GrexExpressionContext *context = GREX_EXPRESSION_CONTEXT(object);

  g_clear_object(&context->scope);
  g_clear_pointer(&context->extra_names, g_hash_table_unref);
}

static void
grex_expression_context_class_init(GrexExpressionContextClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_expression_context_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_SCOPE] = g_param_spec_object(
      "scope", "Scope", "The primary lookup scope.", G_TYPE_OBJECT,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  gpropz_install_property(object_class, GrexExpressionContext, scope,
                          PROP_SCOPE, properties[PROP_SCOPE], NULL);

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
  context->extra_names = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                               (GDestroyNotify)destroy_gvalue);
}

/**
 * grex_expression_context_new:
 * @scope: (transfer none) (nullable): The scope to look up values in.
 *
 * Creates a new context to evaluate an expression, with an empty global scope.
 *
 * Returns: (transfer full): The new expression context.
 */
GrexExpressionContext *
grex_expression_context_new(GObject *scope) {
  return g_object_new(GREX_TYPE_EXPRESSION_CONTEXT, "scope", scope, NULL);
}

/**
 * grex_expression_context_clone:
 * @scope: (transfer none) (nullable): The context to clone.
 *
 * Clones an expression context, including the extra names inside it.
 *
 * Returns: (transfer full): The new expression context.
 */
GrexExpressionContext *
grex_expression_context_clone(GrexExpressionContext *base) {
  GrexExpressionContext *context = grex_expression_context_new(base->scope);

  context->extra_names = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                               (GDestroyNotify)destroy_gvalue);
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, base->extra_names);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    grex_expression_context_insert(context, key, value);
  }

  return context;
}

/**
 * grex_expression_context_get_scope:
 *
 * Returns: (transfer none): The scope.
 */
GPROPZ_DEFINE_RO(GObject *, GrexExpressionContext, grex_expression_context,
                 scope, properties[PROP_SCOPE])

/**
 * grex_expression_context_reset_dependencies:
 *
 * Resets all the expression dependencies currently emitting "changed" signals.
 */
void
grex_expression_context_reset_dependencies(GrexExpressionContext *context) {
  g_signal_emit(context, signals[SIGNAL_RESET], 0);
}

/**
 * grex_expression_context_insert:
 * @name: The name to use.
 * @value: The value to insert.
 *
 * Adds the given object to the list of scopes in this context.

 * Returns: %TRUE if the name did not exist yet.
 */
gboolean
grex_expression_context_insert(GrexExpressionContext *context, const char *name,
                               const GValue *value) {
  GValue *cloned_value = g_new0(GValue, 1);
  g_value_init(cloned_value, value->g_type);
  g_value_copy(value, cloned_value);

  gboolean newly_inserted =
      g_hash_table_insert(context->extra_names, g_strdup(name), cloned_value);
  grex_expression_context_emit_changed(context);
  return newly_inserted;
}

/**
 * grex_expression_context_find_name:
 * @name: The name to find.
 * @dest: The destination to store the value in.
 * @originating_scope: (out) (nullable): The originating scope for this value.
 *
 * Attempts to find the given name in both the extra names table and this
 * context's scope. If found in the scope, it will be assigned to the
 * originating scope passed in.
 *
 * Returns: %TRUE if the name was found.
 */
gboolean
grex_expression_context_find_name(GrexExpressionContext *context,
                                  const char *name, GValue *dest,
                                  GObject **originating_scope) {
  GValue *value = g_hash_table_lookup(context->extra_names, name);
  if (value != NULL) {
    g_value_init(dest, value->g_type);
    g_value_copy(value, dest);
    return TRUE;
  }

  if (context->scope != NULL) {
    GObjectClass *scope_class = G_OBJECT_GET_CLASS(context->scope);
    if (g_object_class_find_property(scope_class, name) != NULL) {
      g_object_get_property(context->scope, name, dest);
      if (originating_scope != NULL) {
        *originating_scope = g_object_ref(context->scope);
      }
      return TRUE;
    }
  }

  return FALSE;
}

void
grex_expression_context_emit_changed(GrexExpressionContext *context) {
  g_object_freeze_notify(G_OBJECT(context));
  g_signal_emit(context, signals[SIGNAL_CHANGED], 0);
  g_object_thaw_notify(G_OBJECT(context));
}
