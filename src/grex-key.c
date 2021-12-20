/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-key.h"

#include "grex-key-private.h"

#include <inttypes.h>

typedef enum { KEY_STRING, KEY_OBJECT } KeyType;

struct _GrexKey {
  grefcount rc;

  GQuark ns;
  KeyType key_type;
  gpointer key;
};

G_DEFINE_BOXED_TYPE(GrexKey, grex_key, grex_key_ref, grex_key_unref)

/**
 * grex_key_new_string:
 * @ns: The key's namespace.
 * @inner: The string to store in this key.
 *
 * Creates a new #GrexKey in the given namespace containing a string.
 *
 * Returns: (transfer full): The new key.
 */
GrexKey *
grex_key_new_string(GQuark ns, const char *inner) {
  GrexKey *key = g_new0(GrexKey, 1);
  g_ref_count_init(&key->rc);

  key->ns = ns;
  key->key_type = KEY_STRING;
  key->key = g_strdup(inner);

  return key;
}

/**
 * grex_key_new_object:
 * @ns: The key's namespace.
 * @inner: The object to store in this key.
 *
 * Creates a new #GrexKey in the given namespace containing an object.
 *
 * Returns: (transfer full): The new key.
 */
GrexKey *
grex_key_new_object(GQuark ns, GObject *inner) {
  GrexKey *key = g_new0(GrexKey, 1);
  g_ref_count_init(&key->rc);

  key->ns = ns;
  key->key_type = KEY_OBJECT;
  key->key = g_object_ref(inner);

  return key;
}

/**
 * grex_key_ref:
 *
 * Increases the key's reference count.
 *
 * Returns: The same key.
 */
GrexKey *
grex_key_ref(GrexKey *key) {
  g_ref_count_inc(&key->rc);
  return key;
}

/**
 * grex_key_unref:
 *
 * Decreases the key's reference count. When the reference count drops to 0, the
 * memory used by the key is freed.
 */
void
grex_key_unref(GrexKey *key) {
  if (g_ref_count_dec(&key->rc)) {
    switch (key->key_type) {
    case KEY_OBJECT:
      g_object_unref(key->key);
      break;
    case KEY_STRING:
      g_free(key->key);
      break;
    }
  }
}

/**
 * grex_key_equals:
 * @a: The first key.
 * @b: The second key.
 *
 * Compare the two keys.
 *
 * Returns: %TRUE if the keys are considered equal.
 */
gboolean
grex_key_equals(const GrexKey *a, const GrexKey *b) {
  if (a->ns != b->ns || a->key_type != b->key_type) {
    return FALSE;
  }

  switch (a->key_type) {
  case KEY_OBJECT:
    return a->key == b->key;
  case KEY_STRING:
    return g_str_equal(a->key, b->key);
  }
}

// Standard FNV-1a hash.

#define FNV_OFFSET_BASIS 0x811c9dc5
#define FNV_PRIME 0x01000193

static void
fnv1a_update(guint *hash, guint8 *bytes, gsize len) {
  for (gsize i = 0; i < len; i++) {
    *hash ^= bytes[i];
    *hash *= FNV_PRIME;
  }
}

guint
grex_key_hash(const GrexKey *key) {
  guint hash = FNV_OFFSET_BASIS;
  fnv1a_update(&hash, (guint8 *)&key->ns, sizeof(key->ns));

  switch (key->key_type) {
  case KEY_OBJECT:
    fnv1a_update(&hash, (guint8 *)&key->key, sizeof(key->key));
    break;
  case KEY_STRING:
    fnv1a_update(&hash, (guint8 *)key->key, strlen(key->key));
    break;
  }

  return hash;
}

char *
grex_key_describe(const GrexKey *key) {
  const char *ns = g_quark_to_string(key->ns);
  switch (key->key_type) {
  case KEY_STRING:
    return g_strdup_printf("%s:%s", ns, (const char *)key->key);
  case KEY_OBJECT:
    return g_strdup_printf("%s:0x%" PRIXPTR, ns, (uintptr_t)key->key);
  }
}

G_DEFINE_QUARK("grex-private-key-namespace-quark", grex_private_key_namespace)
