/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-value-holder.h"

struct _GrexValueHolder {
  grefcount rc;

  GValue value;

  struct {
    GrexValueHolderPushHandler func;
    gpointer data;
    GDestroyNotify data_free;
  } push_handler;
};

G_DEFINE_BOXED_TYPE(GrexValueHolder, grex_value_holder, grex_value_holder_ref,
                    grex_value_holder_unref)

/**
 * grex_value_holder_new:
 * @value: The value to store in this holder.
 *
 * Creates a new #GrexValueHolder that does not support pushing new values.
 *
 * Returns: (transfer full): The new value holder.
 */
GrexValueHolder *
grex_value_holder_new(const GValue *value) {
  return grex_value_holder_new_with_push_handler(value, NULL, NULL, NULL);
}

/**
 * grex_value_holder_new_with_push_handler:
 * @value: The value to store in this holder.
 * @handler: (scope notified) (closure handler_data) (destroy handler_data_free)
 *           (nullable): A callback to push new values.
 * @handler_data: (nullable): User data to pass to the push handler.
 * @handler_data_free: (nullable): Destroy notify for @handler_data.
 *
 * Creates a new #GrexValueHolder that may have new values pushed into it, if
 * @handler is not NULL.
 *
 * Returns: (transfer full): The new value holder.
 */
GrexValueHolder *
grex_value_holder_new_with_push_handler(const GValue *value,
                                        GrexValueHolderPushHandler handler,
                                        gpointer handler_data,
                                        GDestroyNotify handler_data_free) {
  GrexValueHolder *holder = g_new0(GrexValueHolder, 1);
  g_ref_count_init(&holder->rc);

  g_value_init(&holder->value, G_VALUE_TYPE(value));
  g_value_copy(value, &holder->value);

  holder->push_handler.func = handler;
  holder->push_handler.data = handler_data;
  holder->push_handler.data_free = handler_data_free;

  return holder;
}

/**
 * grex_value_holder_ref:
 *
 * Increases the value holder's reference count.
 *
 * Returns: The same value holder.
 */
GrexValueHolder *
grex_value_holder_ref(GrexValueHolder *holder) {
  g_ref_count_inc(&holder->rc);
  return holder;
}

/**
 * grex_value_holder_unref:
 *
 * Decreases the value holder's reference count. When the reference count drops
 * to 0, the memory used by the value holder and its push handler data is freed.
 */
void
grex_value_holder_unref(GrexValueHolder *holder) {
  if (g_ref_count_dec(&holder->rc)) {
    grex_value_holder_disable_push(holder);
    g_value_unset(&holder->value);
  }
}

/**
 * grex_value_holder_get_value:
 *
 * Retrieves the value inside this value holder.
 *
 * Returns: (transfer none): The value holder's value.
 */
const GValue *
grex_value_holder_get_value(GrexValueHolder *holder) {
  return &holder->value;
}

/**
 * grex_value_holder_can_push:
 *
 * Determines if this value holder can have new values pushed to it.
 *
 * Returns: TRUE if it can have new values pushed.
 */
gboolean
grex_value_holder_can_push(GrexValueHolder *holder) {
  return holder->push_handler.func != NULL;
}

/**
 * grex_value_holder_disable_push:
 *
 * Disables all support for pushing new values into this value holder. If no
 * push support was available, does nothing.
 */
void
grex_value_holder_disable_push(GrexValueHolder *holder) {
  holder->push_handler.func = NULL;
  if (holder->push_handler.data_free != NULL) {
    GDestroyNotify destructor =
        g_steal_pointer(&holder->push_handler.data_free);
    destructor(g_steal_pointer(&holder->push_handler.data));
  }
}

/**
 * grex_value_holder_push:
 *
 * Pushes a new #GValue into this value holder. It is an error to call this if
 * the value holder does not support pushing new values into it.
 */
void
grex_value_holder_push(GrexValueHolder *holder, const GValue *value) {
  g_return_if_fail(holder->push_handler.func != NULL);
  holder->push_handler.func(value, holder->push_handler.data);
}
