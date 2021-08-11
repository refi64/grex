/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-inflator.h"

#include "grex-fragment-host.h"
#include "grex-property-set.h"

struct _GrexInflator {
  GObject parent_instance;

  GHashTable *directive_types;
  GHashTable *auto_directive_names;
};

G_DEFINE_TYPE(GrexInflator, grex_inflator, G_TYPE_OBJECT)

static void
grex_inflator_finalize(GObject *object) {
  GrexInflator *inflator = GREX_INFLATOR(object);

  g_clear_pointer(&inflator->auto_directive_names,  // NOLINT
                  g_hash_table_unref);
  g_clear_pointer(&inflator->directive_types, g_hash_table_unref);  // NOLINT
}

static void
grex_inflator_class_init(GrexInflatorClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = grex_inflator_finalize;
}

static void
grex_inflator_init(GrexInflator *inflator) {
  inflator->directive_types = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                    g_free, g_type_class_unref);
  inflator->auto_directive_names = g_hash_table_new(g_str_hash, g_str_equal);
}

/**
 * grex_inflator_new:
 *
 * Creates a new #GrexInflator.
 *
 * Returns: (transfer full): A new inflator.
 */
GrexInflator *
grex_inflator_new() {
  return g_object_new(GREX_TYPE_INFLATOR, NULL);
}

void
grex_inflator_add_directives(GrexInflator *inflator,
                             GrexInflatorDirectiveFlags flags, ...) {
  g_autoptr(GArray) directives = g_array_new(FALSE, TRUE, sizeof(GType));

  va_list va;
  va_start(va, flags);
  for (;;) {
    GType directive_type = va_arg(va, GType);
    if (directive_type == 0) {
      break;
    }
    g_array_append_val(directives, directive_type);
  }
  va_end(va);

  grex_inflator_add_directivesv(inflator, flags, directives->len,
                                (GType *)directives->data);
}

/**
 * grex_inflator_add_directivesv: (rename-to grex_inflator_add_directives)
 * @directives: (array length=n_directives)
 */
void
grex_inflator_add_directivesv(GrexInflator *inflator,
                              GrexInflatorDirectiveFlags flags,
                              guint n_directives, GType *directives) {
  for (guint i = 0; i < n_directives; i++) {
    GType type = directives[i];
    g_autoptr(GrexDirectiveClass) directive_class =
        GREX_DIRECTIVE_CLASS(g_type_class_ref(type));
    const char *name = grex_directive_class_get_name(directive_class);
    if (name == NULL) {
      g_warning("Directive '%s' has no name set", g_type_name(type));
      continue;
    }

    char *dup_name = g_strdup(name);
    g_hash_table_insert(inflator->directive_types, dup_name,
                        g_steal_pointer(&directive_class));
    if (!(flags & GREX_INFLATOR_DIRECTIVE_NO_AUTO_ATTACH)) {
      g_hash_table_add(inflator->auto_directive_names, dup_name);
    }
  }
}

static GrexPropertySet *
grex_inflator_evaluate_fragment_property_set(GrexInflator *inflator,
                                             GrexFragment *fragment) {
  g_autoptr(GrexPropertySet) properties = grex_property_set_new();
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);

  for (GList *target = targets; target != NULL; target = target->next) {
    if (g_hash_table_contains(inflator->directive_types, target->data)) {
      // Skip it, it's a directive that is handled separately.
      continue;
    }

    g_autoptr(GError) error = NULL;
    GrexBinding *binding = grex_fragment_get_binding(fragment, target->data);

    g_auto(GValue) value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_autoptr(GrexValueHolder) result = grex_binding_evaluate(binding, &error);
    if (result == NULL) {
      GrexSourceLocation *location = grex_binding_get_location(binding);
      g_autofree char *location_string = grex_source_location_format(location);
      g_warning("%s: Failed to evaluate binding: %s", location_string,
                error->message);
      continue;
    }

    grex_property_set_insert(properties, target->data, result);
  }

  return g_steal_pointer(&properties);
}

static void
add_directive(GrexFragmentHost *host, GrexDirectiveClass *directive_class,
              GHashTable *inserted_directives) {
  g_autoptr(GrexAttributeDirective) directive =
      grex_fragment_host_get_leftover_attribute_directive(
          host, G_TYPE_FROM_CLASS(directive_class));
  if (directive != NULL) {
    g_object_ref(directive);
  } else {
    directive = g_object_new(G_TYPE_FROM_CLASS(directive_class), NULL);
  }

  grex_fragment_host_add_attribute_directive(host, directive);
  grex_fragment_host_apply_pending_directive_updates(host);

  g_hash_table_add(inserted_directives, directive_class);
}

static void
grex_inflator_auto_attach_directives(GrexInflator *inflator,
                                     GrexFragmentHost *host,
                                     GrexFragment *fragment,
                                     GHashTable *inserted_directives) {
  GHashTableIter iter;
  gpointer key;

  g_hash_table_iter_init(&iter, inflator->auto_directive_names);
  while (g_hash_table_iter_next(&iter, &key, NULL)) {
    const char *name = key;
    GrexDirectiveClass *directive_class =
        g_hash_table_lookup(inflator->directive_types, name);
    if (G_UNLIKELY(directive_class == NULL)) {
      g_warning("Missing auto-assign directive: %s", name);
      continue;
    }

    GType auto_attach_type =
        grex_directive_class_get_auto_attach(directive_class);

    if (auto_attach_type != 0 &&
        g_type_is_a(grex_fragment_get_widget_type(fragment),
                    auto_attach_type) &&
        !g_hash_table_contains(inserted_directives, directive_class)) {
      add_directive(host, directive_class, inserted_directives);
    }
  }
}

static void
grex_inflator_apply_directives(GrexInflator *inflator, GrexFragmentHost *host,
                               GrexFragment *fragment) {
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);
  g_autoptr(GHashTable) inserted_directives =
      g_hash_table_new(g_str_hash, g_str_equal);

  for (GList *target = targets; target != NULL; target = target->next) {
    const char *name = target->data;
    GrexDirectiveClass *directive_class =
        g_hash_table_lookup(inflator->directive_types, name);
    if (directive_class == NULL) {
      // Not a directive.
      continue;
    }

    add_directive(host, directive_class, inserted_directives);
  }

  grex_inflator_auto_attach_directives(inflator, host, fragment,
                                       inserted_directives);
}

/**
 * grex_inflator_inflate_new_widget:
 * @fragment: (transfer none): The fragment to inflate.
 *
 * Creates a new #GtkWidget, inflating the given fragment into it.
 *
 * Returns: (transfer full): The newly created widget.
 */
GtkWidget *
grex_inflator_inflate_new_widget(GrexInflator *inflator,
                                 GrexFragment *fragment) {
  GType widget_type = grex_fragment_get_widget_type(fragment);
  g_return_val_if_fail(g_type_is_a(widget_type, GTK_TYPE_WIDGET), NULL);

  // TODO: handle construct-only properties.
  GtkWidget *widget = GTK_WIDGET(g_object_new(widget_type, NULL));
  grex_inflator_inflate_existing_widget(inflator, widget, fragment);
  return widget;
}

/**
 * grex_inflator_inflate_existing_widget:
 * @fragment: (transfer none): The fragment to inflate.
 * @widget: (transfer none): The widget to inflate the fragent into.
 *
 * Inflates the given fragment into the given widget.
 */
void
grex_inflator_inflate_existing_widget(GrexInflator *inflator, GtkWidget *widget,
                                      GrexFragment *fragment) {
  g_autoptr(GrexFragmentHost) host = grex_fragment_host_for_widget(widget);
  if (host == NULL) {
    host = grex_fragment_host_new(widget);
  } else {
    g_object_ref(host);
    g_return_if_fail(grex_fragment_host_matches_fragment_type(host, fragment));
  }

  g_autoptr(GrexPropertySet) properties =
      grex_inflator_evaluate_fragment_property_set(inflator, fragment);
  grex_fragment_host_apply_latest_properties(host, properties);

  grex_fragment_host_begin_inflation(host);

  grex_inflator_apply_directives(inflator, host, fragment);

  g_autoptr(GList) children = grex_fragment_get_children(fragment);
  for (GList *child = children; child != NULL; child = child->next) {
    grex_inflator_inflate_child(inflator, host, (guintptr)child->data,
                                child->data);
  }

  grex_fragment_host_commit_inflation(host);
}

void
grex_inflator_inflate_child(GrexInflator *inflator, GrexFragmentHost *parent,
                            guintptr key, GrexFragment *child) {
  GtkWidget *child_widget = grex_fragment_host_get_leftover_child(parent, key);
  if (child_widget == NULL) {
    child_widget = grex_inflator_inflate_new_widget(inflator, child);
  } else {
    grex_inflator_inflate_existing_widget(inflator, child_widget, child);
  }

  grex_fragment_host_add_inflated_child(parent, key, child_widget);
}
