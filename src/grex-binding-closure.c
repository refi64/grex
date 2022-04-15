/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-binding-closure-private.h"

struct _GrexBindingClosure {
  GClosure closure;

  GrexBinding *binding;
  GrexExpressionContext *context;
};

static void
grex_binding_closure_finalize(gpointer notify_data, GClosure *closure) {
  GrexBindingClosure *binding_closure = (GrexBindingClosure *)closure;

  g_clear_object(&binding_closure->binding);
  g_clear_object(&binding_closure->context);
}

static void
grex_binding_closure_marshal(GClosure *closure, GValue *return_value,
                             guint n_params, const GValue *params,
                             gpointer invocation_hint, gpointer marshal_data) {
  GrexBindingClosure *binding_closure = (GrexBindingClosure *)closure;

  // XXX: should be using proper child contexts once they're implemented.
  g_autoptr(GrexExpressionContext) context = grex_expression_context_new(
      grex_expression_context_get_scope(binding_closure->context));
  for (guint i = 0; i < n_params; i++) {
    g_autofree char *name = g_strdup_printf("$%u", i);
    grex_expression_context_insert(context, name, &params[i]);
  }

  g_autoptr(GError) error = NULL;
  g_autoptr(GrexValueHolder) result = grex_binding_evaluate(
      binding_closure->binding,
      return_value != NULL ? G_VALUE_TYPE(return_value) : G_TYPE_NONE, context,
      FALSE, &error);
  if (!result) {
    g_warning("Failed to evaluate binding closure: %s", error->message);
  } else if (return_value != NULL) {
    g_value_copy(grex_value_holder_get_value(result), return_value);
  }
}

GClosure *
grex_binding_closure_create(GrexBinding *binding,
                            GrexExpressionContext *context) {
  GClosure *closure = g_closure_new_simple(sizeof(GrexBindingClosure), NULL);

  GrexBindingClosure *binding_closure = (GrexBindingClosure *)closure;
  binding_closure->binding = g_object_ref(binding);
  binding_closure->context = g_object_ref(context);

  g_closure_set_marshal(closure, grex_binding_closure_marshal);
  g_closure_add_finalize_notifier(closure, NULL, grex_binding_closure_finalize);
  return closure;
}
