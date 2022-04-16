/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "grex-config.h"

G_BEGIN_DECLS

#define GREX_TYPE_KEY grex_key_get_type()

typedef struct _GrexKey GrexKey;
GType grex_key_get_type();

GrexKey *grex_key_new_int(GQuark ns, int inner);
GrexKey *grex_key_new_string(GQuark ns, const char *inner);
GrexKey *grex_key_new_object(GQuark ns, GObject *inner);

GrexKey *grex_key_ref(GrexKey *source);
void grex_key_unref(GrexKey *key);

gboolean grex_key_equals(const GrexKey *a, const GrexKey *b);
guint grex_key_hash(const GrexKey *key);

char *grex_key_describe(const GrexKey *key);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GrexKey, grex_key_unref)

G_END_DECLS
