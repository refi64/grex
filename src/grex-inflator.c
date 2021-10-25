/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-inflator.h"

#include "gpropz.h"
#include "grex-fragment-host.h"
#include "grex-structural-directive.h"

#define g_object_ref0(obj) \
  ({                       \
    if (obj != NULL) {     \
      g_object_ref(obj);   \
    }                      \
    obj;                   \
  })

struct _GrexInflator {
  GObject parent_instance;

  GrexExpressionContext *context;

  GHashTable *directive_factories;
  GHashTable *auto_directive_names;
};

enum {
  PROP_CONTEXT = 1,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexInflator, grex_inflator, G_TYPE_OBJECT)

static void
grex_inflator_finalize(GObject *object) {
  GrexInflator *inflator = GREX_INFLATOR(object);

  g_clear_pointer(&inflator->auto_directive_names,  // NOLINT
                  g_hash_table_unref);
  g_clear_pointer(&inflator->directive_factories,  // NOLINT
                  g_hash_table_unref);
}

static void
grex_inflator_class_init(GrexInflatorClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = grex_inflator_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_CONTEXT] = g_param_spec_object(
      "context", "Evaluation context",
      "The context used to evaluation expressions.",
      GREX_TYPE_EXPRESSION_CONTEXT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexInflator, context, PROP_CONTEXT,
                          properties[PROP_CONTEXT], NULL);
}

static void
grex_inflator_init(GrexInflator *inflator) {
  inflator->directive_factories =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
  inflator->auto_directive_names = g_hash_table_new(g_str_hash, g_str_equal);
}

/**
 * grex_inflator_new:
 * @context: The context used to evaluate expressions.
 *
 * Creates a new #GrexInflator.
 *
 * Returns: (transfer full): A new inflator.
 */
GrexInflator *
grex_inflator_new(GrexExpressionContext *context) {
  return g_object_new(GREX_TYPE_INFLATOR, "context", context, NULL);
}

/**
 * grex_inflator_new_with_scope:
 * @scope: The scope to look up expression values in.
 *
 * Creates a new #GrexInflator, creating a new #GrexExpressionContext with the
 * given scope in the process.
 *
 * Returns: (transfer full): A new inflator.
 */
GrexInflator *
grex_inflator_new_with_scope(GObject *scope) {
  g_autoptr(GrexExpressionContext) context = grex_expression_context_new();
  grex_expression_context_add_scope(context, scope);
  return grex_inflator_new(context);
}

/**
 * grex_inflator_get_context:
 *
 * Returns the expression context used to evaluate bindings.
 *
 * Returns: (transfer none): The expression context.
 */
GPROPZ_DEFINE_RO(GrexExpressionContext *, GrexInflator, grex_inflator, context,
                 properties[PROP_CONTEXT])

void
grex_inflator_take_directives(GrexInflator *inflator,
                              GrexInflatorDirectiveFlags flags, ...) {
  g_autoptr(GPtrArray) directives =
      g_ptr_array_new_with_free_func(g_object_unref);

  va_list va;
  va_start(va, flags);
  for (;;) {
    GrexDirectiveFactory *factory = va_arg(va, GrexDirectiveFactory *);
    if (factory == NULL) {
      break;
    }

    g_ptr_array_add(directives, factory);
  }
  va_end(va);

  grex_inflator_add_directivesv(inflator, flags, directives->len,
                                (GrexDirectiveFactory **)directives->pdata);
}

void
grex_inflator_add_directives(GrexInflator *inflator,
                             GrexInflatorDirectiveFlags flags, ...) {
  g_autoptr(GPtrArray) directives = g_ptr_array_new();

  va_list va;
  va_start(va, flags);
  for (;;) {
    GrexDirectiveFactory *factory = va_arg(va, GrexDirectiveFactory *);
    if (factory == NULL) {
      break;
    }

    g_ptr_array_add(directives, factory);
  }
  va_end(va);

  grex_inflator_add_directivesv(inflator, flags, directives->len,
                                (GrexDirectiveFactory **)directives->pdata);
}

/**
 * grex_inflator_add_directivesv: (rename-to grex_inflator_add_directives)
 * @directives: (array length=n_directives) (transfer none)
 */
void
grex_inflator_add_directivesv(GrexInflator *inflator,
                              GrexInflatorDirectiveFlags flags,
                              guint n_directives,
                              GrexDirectiveFactory **directives) {
  for (guint i = 0; i < n_directives; i++) {
    GrexDirectiveFactory *factory = directives[i];

    char *name = g_strdup(grex_directive_factory_get_name(factory));
    g_hash_table_insert(inflator->directive_factories, name,
                        g_object_ref(factory));

    if (!(flags & GREX_INFLATOR_DIRECTIVE_NO_AUTO_ATTACH) &&
        GREX_IS_PROPERTY_DIRECTIVE_FACTORY(factory)) {
      g_hash_table_add(inflator->auto_directive_names, name);
    }
  }
}

static inline const char *
parse_property_directive_name(const char *name) {
  if (name[0] == '_' && name[1] != '_') {
    return name + 1;
  } else {
    return NULL;
  }
}

static inline const char *
parse_structural_directive_name(const char *name) {
  if (g_str_has_prefix(name, "__")) {
    return name + 2;
  } else {
    return NULL;
  }
}

static void
grex_inflator_apply_binding(GrexInflator *inflator, GrexFragmentHost *host,
                            const char *name, GrexBinding *binding,
                            gboolean track_dependencies) {
  g_autoptr(GError) error = NULL;
  g_autoptr(GrexValueHolder) result = grex_binding_evaluate(
      binding, inflator->context, track_dependencies, &error);
  if (result == NULL) {
    GrexSourceLocation *location = grex_binding_get_location(binding);
    g_autofree char *location_string = grex_source_location_format(location);
    g_warning("%s: Failed to evaluate binding: %s", location_string,
              error->message);
    return;
  }

  grex_fragment_host_add_property(host, name, result);
}

static void
grex_inflator_apply_properties(GrexInflator *inflator, GrexFragmentHost *host,
                               GrexFragment *fragment,
                               gboolean track_dependencies) {
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);

  for (GList *target = targets; target != NULL; target = target->next) {
    const char *name = target->data;
    if (parse_property_directive_name(name) != NULL ||
        parse_structural_directive_name(name) != NULL) {
      // Skip it, it's a directive that is handled separately.
      continue;
    }

    grex_inflator_apply_binding(inflator, host, name,
                                grex_fragment_get_binding(fragment, name),
                                track_dependencies);
  }
}

static gboolean
grex_inflator_get_directive_and_property(GrexInflator *inflator,
                                         const char *name,
                                         GrexDirectiveFactory **out_factory,
                                         const char **out_property) {
  g_return_val_if_fail(out_factory != NULL, FALSE);
  g_return_val_if_fail(out_property != NULL, FALSE);

  *out_factory = g_hash_table_lookup(inflator->directive_factories, name);

  if (*out_factory == NULL) {
    // Try to find one with the last element of the name removed, using that
    // as the property name.
    const char *dot = strrchr(name, '.');
    if (dot == NULL) {
      g_warning("Unknown directive: '%s'", name);
      return FALSE;
    }

    g_autofree char *name_without_property = g_strndup(name, dot - name);
    *out_factory = g_hash_table_lookup(inflator->directive_factories,
                                       name_without_property);
    if (*out_factory == NULL) {
      g_warning("Unknown directive: '%s' or '%s'", name, name_without_property);
      return FALSE;
    }

    if (grex_directive_factory_get_property_format(*out_factory) !=
        GREX_DIRECTIVE_PROPERTY_FORMAT_EXPLICIT) {
      g_warning("Directive '%s' does not take any explicitly named properties",
                name_without_property);
      return FALSE;
    }

    *out_property = dot + 1;
  } else {
    switch (grex_directive_factory_get_property_format(*out_factory)) {
    case GREX_DIRECTIVE_PROPERTY_FORMAT_NONE:
      // Nothing to assign.
      *out_property = NULL;
      break;
    case GREX_DIRECTIVE_PROPERTY_FORMAT_IMPLICIT_VALUE:
      *out_property = "value";
      break;
    case GREX_DIRECTIVE_PROPERTY_FORMAT_EXPLICIT:
      g_warning("Directive '%s' requires a property name to set", name);
      return FALSE;
    }
  }

  return TRUE;
}

static GrexPropertyDirective *
add_property_directive(GrexFragmentHost *host,
                       GrexPropertyDirectiveFactory *factory,
                       GHashTable *inserted_directives) {
  g_autoptr(GrexPropertyDirective) directive =
      g_object_ref0(g_hash_table_lookup(inserted_directives, factory));
  if (directive != NULL) {
    return g_steal_pointer(&directive);
  }

  directive = g_object_ref0(grex_fragment_host_get_leftover_property_directive(
      host, (guintptr)factory));
  if (directive == NULL) {
    directive = grex_property_directive_factory_create(factory);
    g_return_val_if_fail(directive != NULL, NULL);

    g_object_unref(grex_fragment_host_new(G_OBJECT(directive)));
  }

  grex_fragment_host_add_property_directive(host, (guintptr)factory, directive);
  g_hash_table_insert(inserted_directives, factory, directive);

  grex_fragment_host_begin_inflation(
      grex_fragment_host_for_target(G_OBJECT(directive)));
  return directive;
}

static void
grex_inflator_apply_explicit_directives(GrexInflator *inflator,
                                        GrexFragmentHost *host,
                                        GrexFragment *fragment,
                                        GHashTable *inserted_directives,
                                        gboolean track_dependencies) {
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(fragment);
  for (GList *target = targets; target != NULL; target = target->next) {
    const char *name = target->data;
    const char *unprefixed_name = parse_property_directive_name(name);
    if (unprefixed_name == NULL) {
      continue;
    }

    GrexDirectiveFactory *factory = NULL;
    const char *property = NULL;
    if (!grex_inflator_get_directive_and_property(inflator, unprefixed_name,
                                                  &factory, &property)) {
      continue;
    }

    if (!GREX_IS_PROPERTY_DIRECTIVE_FACTORY(factory)) {
      g_warning("'%s' was expected to be a property directive",
                grex_directive_factory_get_name(factory));
      continue;
    }

    GrexBinding *binding = grex_fragment_get_binding(fragment, name);

    GrexPropertyDirective *directive = add_property_directive(
        host, GREX_PROPERTY_DIRECTIVE_FACTORY(factory), inserted_directives);
    if (property != NULL) {
      GrexFragmentHost *directive_host =
          grex_fragment_host_for_target(G_OBJECT(directive));
      grex_inflator_apply_binding(inflator, directive_host, property, binding,
                                  track_dependencies);
    }
  }
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
    GrexPropertyDirectiveFactory *factory =
        g_hash_table_lookup(inflator->directive_factories, name);
    if (G_UNLIKELY(factory == NULL)) {
      g_warning("Missing auto-assign directive: %s", name);
      continue;
    }

    if (grex_property_directive_factory_should_auto_attach(factory, host,
                                                           fragment)) {
      if (grex_directive_factory_get_property_format(GREX_DIRECTIVE_FACTORY(
              factory)) == GREX_DIRECTIVE_PROPERTY_FORMAT_EXPLICIT) {
        g_warning(
            "Cannot auto-assign directive '%s' requiring explicit properties",
            name);
        continue;
      }

      GrexPropertyDirective *directive =
          add_property_directive(host, factory, inserted_directives);
      GrexFragmentHost *directive_host =
          grex_fragment_host_for_target(G_OBJECT(directive));

      // If this requires a value, pass in the empty string.
      if (grex_directive_factory_get_property_format(GREX_DIRECTIVE_FACTORY(
              factory)) == GREX_DIRECTIVE_PROPERTY_FORMAT_IMPLICIT_VALUE) {
        GValue value = G_VALUE_INIT;
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_static_string(&value, "");

        g_autoptr(GrexBindingBuilder) binding_builder =
            grex_binding_builder_new();
        grex_binding_builder_add_constant(binding_builder, "", -1);
        g_autoptr(GrexBinding) binding = grex_binding_builder_build(
            binding_builder, grex_fragment_get_location(fragment));

        grex_inflator_apply_binding(inflator, directive_host, "value", binding,
                                    FALSE);
      }
    }
  }
}

static void
commit_directives(GHashTable *inserted_directives) {
  GHashTableIter iter;
  g_hash_table_iter_init(&iter, inserted_directives);

  gpointer directive = NULL;
  while (g_hash_table_iter_next(&iter, NULL, &directive)) {
    GrexFragmentHost *host = grex_fragment_host_for_target(directive);
    grex_fragment_host_commit_inflation(host);
  }
}

static void
grex_inflator_apply_directives(GrexInflator *inflator, GrexFragmentHost *host,
                               GrexFragment *fragment,
                               gboolean track_dependencies) {
  g_autoptr(GHashTable) inserted_directives =
      g_hash_table_new(g_str_hash, g_str_equal);

  grex_inflator_apply_explicit_directives(
      inflator, host, fragment, inserted_directives, track_dependencies);
  grex_inflator_auto_attach_directives(inflator, host, fragment,
                                       inserted_directives);
  commit_directives(inserted_directives);
}

/**
 * grex_inflator_inflate_new_target:
 * @fragment: (transfer none): The fragment to inflate.
 *
 * Creates a new #GObject, inflating the given fragment into it.
 *
 * Returns: (transfer full): The newly created object.
 */
GObject *
grex_inflator_inflate_new_target(GrexInflator *inflator, GrexFragment *fragment,
                                 GrexInflationFlags flags) {
  GType target_type = grex_fragment_get_target_type(fragment);

  // TODO: handle construct-only properties.
  GObject *target = g_object_new(target_type, NULL);
  grex_inflator_inflate_existing_target(inflator, target, fragment, flags);
  return target;
}

/**
 * grex_inflator_inflate_existing_object:
 * @fragment: (transfer none): The fragment to inflate.
 * @target: (transfer none): The object to inflate the fragent into.
 *
 * Inflates the given fragment into the given object.
 */
void
grex_inflator_inflate_existing_target(GrexInflator *inflator, GObject *target,
                                      GrexFragment *fragment,
                                      GrexInflationFlags flags) {
  g_autoptr(GrexFragmentHost) host =
      g_object_ref0(grex_fragment_host_for_target(target));
  if (host == NULL) {
    host = grex_fragment_host_new(target);
  } else {
    g_return_if_fail(grex_fragment_host_matches_fragment_type(host, fragment));
  }

  grex_fragment_host_begin_inflation(host);

  gboolean track_dependencies = flags & GREX_INFLATION_TRACK_DEPENDENCIES;
  grex_inflator_apply_properties(inflator, host, fragment, track_dependencies);
  grex_inflator_apply_directives(inflator, host, fragment, track_dependencies);

  g_autoptr(GList) children = grex_fragment_get_children(fragment);
  for (GList *child = children; child != NULL; child = child->next) {
    grex_inflator_inflate_child(inflator, host, (guintptr)child->data,
                                child->data, flags, GREX_CHILD_INFLATION_NONE);
  }

  grex_fragment_host_commit_inflation(host);
}

static GrexStructuralDirective *
grex_inflator_find_structural_directive_in_child(GrexInflator *inflator,
                                                 GrexFragmentHost *parent,
                                                 guintptr key,
                                                 GrexFragment *child,
                                                 gboolean track_dependencies) {
  g_autoptr(GList) targets = grex_fragment_get_binding_targets(child);
  GrexDirectiveFactory *current_factory = NULL;
  g_autoptr(GrexStructuralDirective) directive = NULL;

  for (GList *target = targets; target != NULL; target = target->next) {
    const char *name = target->data;
    const char *unprefixed_name = parse_structural_directive_name(name);
    if (unprefixed_name == NULL) {
      continue;
    }

    GrexDirectiveFactory *factory = NULL;
    const char *property = NULL;
    if (!grex_inflator_get_directive_and_property(inflator, unprefixed_name,
                                                  &factory, &property)) {
      continue;
    }

    if (current_factory != NULL && current_factory != factory) {
      g_warning(
          "Cannot have two structural directives '%s' and '%s' on one fragment",
          grex_directive_factory_get_name(current_factory),
          grex_directive_factory_get_name(factory));
      return NULL;
    }

    if (!GREX_IS_STRUCTURAL_DIRECTIVE_FACTORY(factory)) {
      g_warning("'%s' was expected to be a structural directive",
                grex_directive_factory_get_name(factory));
      return NULL;
    }

    current_factory = factory;

    if (directive == NULL) {
      directive = g_object_ref0(
          grex_fragment_host_get_leftover_structural_directive(parent, key));
      if (directive == NULL) {
        directive = grex_structural_directive_factory_create(
            GREX_STRUCTURAL_DIRECTIVE_FACTORY(factory));
        g_return_val_if_fail(directive != NULL, NULL);

        g_object_unref(grex_fragment_host_new(G_OBJECT(directive)));
      }

      grex_fragment_host_add_structural_directive(parent, key, directive);

      grex_fragment_host_begin_inflation(
          grex_fragment_host_for_target(G_OBJECT(directive)));
    }

    if (property != NULL) {
      GrexFragmentHost *directive_host =
          grex_fragment_host_for_target(G_OBJECT(directive));
      GrexBinding *binding = grex_fragment_get_binding(child, name);
      grex_inflator_apply_binding(inflator, directive_host, property, binding,
                                  track_dependencies);
    }
  }

  if (directive != NULL) {
    GrexFragmentHost *directive_host =
        grex_fragment_host_for_target(G_OBJECT(directive));
    grex_fragment_host_commit_inflation(directive_host);
  }

  return g_steal_pointer(&directive);
}

void
grex_inflator_inflate_child(GrexInflator *inflator, GrexFragmentHost *parent,
                            guintptr key, GrexFragment *child,
                            GrexInflationFlags flags,
                            GrexChildInflationFlags child_flags) {
  GObject *child_object = grex_fragment_host_get_leftover_child(parent, key);
  if (child_object == NULL) {
    child_object = grex_inflator_inflate_new_target(inflator, child, flags);
  } else {
    grex_inflator_inflate_existing_target(inflator, child_object, child, flags);
  }

  GrexStructuralDirective *directive = NULL;
  if (!(child_flags & GREX_CHILD_INFLATION_IGNORE_STRUCTURAL_DIRECTIVES) &&
      (directive = grex_inflator_find_structural_directive_in_child(
           inflator, parent, key, child,
           flags & GREX_INFLATION_TRACK_DEPENDENCIES))) {
    GrexStructuralDirectiveClass *directive_class =
        GREX_STRUCTURAL_DIRECTIVE_GET_CLASS(directive);
    directive_class->apply(
        directive, inflator, parent, key, child, flags,
        // Ignore this directive next time to avoid infinite recursion.
        child_flags | GREX_CHILD_INFLATION_IGNORE_STRUCTURAL_DIRECTIVES);
  } else {
    grex_fragment_host_add_inflated_child(parent, key, child_object);
  }
}
