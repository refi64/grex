/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-property-set.h"

struct _GrexPropertySet {
  GObject parent_instance;

  GHashTable *properties;
};

G_DEFINE_TYPE(GrexPropertySet, grex_property_set, G_TYPE_OBJECT)

static void
free_heap_g_value(GValue *value) {
  g_value_unset(value);
  g_free(value);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GValue, free_heap_g_value)

static void
grex_property_set_dispose(GObject *object) {
  GrexPropertySet *properties = GREX_PROPERTY_SET(object);
  g_clear_pointer(&properties->properties, g_hash_table_unref);  // NOLINT
}

static void
grex_property_set_class_init(GrexPropertySetClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_property_set_dispose;
}

static void
grex_property_set_init(GrexPropertySet *properties) {
  properties->properties = g_hash_table_new_full(
      g_str_hash, g_str_equal, g_free, (GDestroyNotify)free_heap_g_value);
}

/**
 * grex_property_set_new:
 *
 * Creates a new, empty #GrexPropertySet.
 *
 * Returns: (transfer full): A new property set.
 */
GrexPropertySet *
grex_property_set_new() {
  return g_object_new(GREX_TYPE_PROPERTY_SET, NULL);
}

/**
 * grex_property_set_get_keys:
 *
 * Returns the keys contained within this property set.
 *
 * Returns: (element-type utf8) (transfer container): A list of keys.
 */
GList *
grex_property_set_get_keys(GrexPropertySet *properties) {
  return g_hash_table_get_keys(properties->properties);
}

/**
 * grex_property_set_add:
 * @name: The property name.
 * @value: The property value.
 *
 * Adds the given property to the set, replacing any previous values if present.
 */
void
grex_property_set_add(GrexPropertySet *properties, const char *name,
                      const GValue *value) {
  g_autoptr(GValue) property_value = g_new0(GValue, 1);
  g_value_init(property_value, G_VALUE_TYPE(value));
  g_value_copy(value, property_value);
  g_hash_table_insert(properties->properties, g_strdup(name),
                      g_steal_pointer(&property_value));
}

/**
 * grex_property_set_contains:
 * @name: The property to search for.
 *
 * Checks if the property set contains the given property.
 *
 * Returns: TRUE if the property set contains the property.
 */
gboolean
grex_property_set_contains(GrexPropertySet *properties, const char *name) {
  return g_hash_table_contains(properties->properties, name);
}

/**
 * grex_property_set_get:
 * @name: The property to get.
 * @value: The #GValue to store the property value in.
 *
 * Searches the property set for the given value, saving it in @value and
 * returning TRUE if found.
 *
 * Returns: TRUE if the property was found.
 */
gboolean
grex_property_set_get(GrexPropertySet *properties, const char *name,
                      GValue *value) {
  const GValue *property_value =
      g_hash_table_lookup(properties->properties, name);
  if (property_value != NULL) {
    if (G_VALUE_TYPE(value) == G_TYPE_INVALID) {
      g_value_init(value, G_VALUE_TYPE(property_value));
    }

    g_value_copy(property_value, value);
    return TRUE;
  }

  return FALSE;
}

/**
 * grex_property_set_remove:
 * @name: The property to remove.
 *
 * Removes the property from this property set.
 *
 * Returns: TRUE if the property was present.
 */
gboolean
grex_property_set_remove(GrexPropertySet *properties, const char *name) {
  return g_hash_table_remove(properties->properties, name);
}

/**
 * grex_property_set_diff_keys:
 * @old_set: The old property set.
 * @new_set: The new property set.
 * @added: (out) (element-type utf8): Will contain property names that were
 *                                    added.
 * @deleted: (out) (element-type utf8): Will contain property names that were
 *                                      removed.
 * @intersection: (out) (element-type utf8): Will contain property names that
 *                                           are present in both sets.
 *
 * Computers the difference between the property names between the old and new
 * sets, saving the list of keys added and deleted in from the old to the new
 * one. Keys present in both are stored in @intersection.
 */
void
grex_property_set_diff_keys(GrexPropertySet *old_set, GrexPropertySet *new_set,
                            GList **added, GList **deleted,
                            GList **intersection) {
  g_return_if_fail(added != NULL);
  g_return_if_fail(deleted != NULL);
  g_return_if_fail(intersection != NULL);

  g_return_if_fail(*added == NULL);
  g_return_if_fail(*deleted == NULL);
  g_return_if_fail(*intersection == NULL);

  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init(&iter, old_set->properties);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    if (!grex_property_set_contains(new_set, key)) {
      *deleted = g_list_prepend(*deleted, key);
    } else {
      *intersection = g_list_prepend(*intersection, key);
    }
  }

  g_hash_table_iter_init(&iter, new_set->properties);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    if (!grex_property_set_contains(old_set, key)) {
      *added = g_list_prepend(*added, key);
    }
  }
}
