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
      g_str_hash, g_str_equal, g_free, (GDestroyNotify)grex_value_holder_unref);
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
 * grex_property_set_insert:
 * @name: The property name.
 * @value: (transfer none): The property value.
 *
 * Inserts the given property ito the set, replacing any previous values if
 * present.
 */
void
grex_property_set_insert(GrexPropertySet *properties, const char *name,
                         GrexValueHolder *value) {
  g_hash_table_insert(properties->properties, g_strdup(name),
                      grex_value_holder_ref(value));
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
 *
 * Searches the property set for the given value, returning it if found.
 *
 * Returns: (transfer none): The value holder, or NULL if not found.
 */
GrexValueHolder *
grex_property_set_get(GrexPropertySet *properties, const char *name) {
  return g_hash_table_lookup(properties->properties, name);
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
 * @added: (out) (element-type utf8) (transfer container): Will contain property
 *         names that were added.
 * @removed: (out) (element-type utf8) (transfer container): Will contain
 *           property names that were removed.
 * @kept: (out) (element-type utf8) (transfer container): Will contain property
 *        names that are present in both sets.
 *
 * Computers the difference between the property names between the old and new
 * sets, saving the list of keys added and removed in from the old to the new
 * one. Keys present in both are stored in @kept.
 */
void
grex_property_set_diff_keys(GrexPropertySet *old_set, GrexPropertySet *new_set,
                            GList **added, GList **removed, GList **kept) {
  g_return_if_fail(added != NULL);
  g_return_if_fail(removed != NULL);
  g_return_if_fail(kept != NULL);

  g_return_if_fail(*added == NULL);
  g_return_if_fail(*removed == NULL);
  g_return_if_fail(*kept == NULL);

  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init(&iter, old_set->properties);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    if (!grex_property_set_contains(new_set, key)) {
      *removed = g_list_prepend(*removed, key);
    } else {
      *kept = g_list_prepend(*kept, key);
    }
  }

  g_hash_table_iter_init(&iter, new_set->properties);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    if (!grex_property_set_contains(old_set, key)) {
      *added = g_list_prepend(*added, key);
    }
  }
}
