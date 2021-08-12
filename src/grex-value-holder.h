/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"

G_BEGIN_DECLS

#define GREX_TYPE_VALUE_HOLDER grex_value_holder_get_type()

typedef struct _GrexValueHolder GrexValueHolder;
GType grex_value_holder_get_type();

typedef void (*GrexValueHolderPushHandler)(const GValue *new_value,
                                           gpointer user_data);

GrexValueHolder *grex_value_holder_new(const GValue *value);

GrexValueHolder *grex_value_holder_new_with_push_handler(
    const GValue *value, GrexValueHolderPushHandler handler,
    gpointer handler_data, GDestroyNotify handler_data_free);

GrexValueHolder *grex_value_holder_ref(GrexValueHolder *holder);
void grex_value_holder_unref(GrexValueHolder *holder);

const GValue *grex_value_holder_get_value(GrexValueHolder *holder);

gboolean grex_value_holder_can_push(GrexValueHolder *holder);
void grex_value_holder_disable_push(GrexValueHolder *holder);
void grex_value_holder_push(GrexValueHolder *value, const GValue *holder);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GrexValueHolder, grex_value_holder_unref)

G_END_DECLS
