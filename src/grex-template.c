/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-template.h"

#include "gpropz.h"

struct _GrexTemplate {
  GObject parent_instance;

  char *resource_path;
  GtkBuilderScope *scope;
  GrexResourceLoader *loader;

  GrexFragment *fragment;
};

enum {
  PROP_FRAGMENT = 1,
  PROP_SCOPE,
  PROP_LOADER,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexTemplate, grex_template, G_TYPE_OBJECT)

G_DEFINE_QUARK("grex-inflator-original-template-quark",
               grex_inflator_original_template)

static void
grex_template_dispose(GObject *object) {
  GrexTemplate *template = GREX_TEMPLATE(object);
  g_clear_object(&template->fragment);
  g_clear_object(&template->scope);
  g_clear_object(&template->loader);
}

static void
grex_template_finalize(GObject *object) {
  GrexTemplate *template = GREX_TEMPLATE(object);
  g_clear_pointer(&template->resource_path, g_free);
}

static void
grex_template_class_init(GrexTemplateClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_template_dispose;
  object_class->finalize = grex_template_finalize;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_FRAGMENT] = g_param_spec_object(
      "fragment", "Fragment", "This template's fragment.", GREX_TYPE_FRAGMENT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexTemplate, fragment, PROP_FRAGMENT,
                          properties[PROP_FRAGMENT], NULL);

  properties[PROP_SCOPE] = g_param_spec_object(
      "scope", "Fragment's scope",
      "The scope used when loaded the given fragment.", GTK_TYPE_BUILDER_SCOPE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexTemplate, scope, PROP_SCOPE,
                          properties[PROP_SCOPE], NULL);

  properties[PROP_LOADER] = g_param_spec_object(
      "loader", "Resource loader.",
      "The resource loader used to load the given fragment.",
      GREX_TYPE_RESOURCE_LOADER, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexTemplate, loader, PROP_LOADER,
                          properties[PROP_LOADER], NULL);
}

static void
grex_template_init(GrexTemplate *template) {
  if (template->loader == NULL) {
    template->loader = g_object_ref(grex_resource_loader_default());
  }
}

/**
 * grex_template_new:
 * @fragment: (transfer none): The fragment to wrap in this template.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names when parsing
 *         the fragment..
 * @loader: (nullable): The #GrexResourceLoader the fragment was loaded from.
 *
 * Creates a new #GrexTemplate containing the given fragment.
 *
 * Returns: (transfer full): A new template.
 */
GrexTemplate *
grex_template_new(GrexFragment *fragment, GtkBuilderScope *scope,
                  GrexResourceLoader *loader) {
  return g_object_new(GREX_TYPE_TEMPLATE, "fragment", fragment, "scope", scope,
                      "loader", loader, NULL);
}

/**
 * grex_template_new_from_xml:
 * @xml: The XML content.
 * @len: Length of @xml in bytes, or -1 if null-terminated.
 * @filename: (nullable): The filename of the content, used in the resulting
 *                        fragment's source location.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names.
 * @loader: (nullable): The #GrexResourceLoader the fragment was loaded from.
 *
 * Creates a new #GrexTemplate containing a fragment parsed from the given XML
 * data.
 *
 * Returns: (transfer full): A new template.
 */
GrexTemplate *
grex_template_new_from_xml(const char *xml, gssize len, const char *filename,
                           GtkBuilderScope *scope, GrexResourceLoader *loader) {
  g_autoptr(GError) error = NULL;
  g_autoptr(GrexFragment) fragment =
      grex_fragment_parse_xml(xml, len, filename, scope, &error);
  if (fragment == NULL) {
    g_critical("Failed to parse template (%s): %s",
               filename != NULL ? filename : "<unknown>", error->message);
    return NULL;
  }

  return grex_template_new(fragment, scope, loader);
}

/**
 * grex_template_new_from_resource:
 * @resource: The resource path.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names.
 * @loader: (nullable): The #GrexResourceLoader the given resource was
 *          registered with.
 *
 * Creates a new #GrexTemplate containing a fragment parsed from the XML data
 * within the given resource path.
 *
 * Returns: (transfer full): A new template.
 */
GrexTemplate *
grex_template_new_from_resource(const char *resource, GtkBuilderScope *scope,
                                GrexResourceLoader *loader) {
  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) bytes =
      g_resources_lookup_data(resource, G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  if (bytes == NULL) {
    g_critical("Failed to look up resource %s: %s", resource, error->message);
    return NULL;
  }

  gsize size = 0;
  const char *xml = g_bytes_get_data(bytes, &size);

  GrexTemplate *template =
      grex_template_new_from_xml(xml, size, resource, scope, loader);
  if (template == NULL) {
    return NULL;
  }

  template->resource_path = g_strdup(resource);
  if (scope != NULL) {
    template->scope = g_object_ref(scope);
  }

  return template;
}

/**
 * grex_template_get_fragment:
 *
 * Returns the #GrexFragment this template wraps.
 *
 * Returns: (transfer none): The fragment.
 */
GPROPZ_DEFINE_RO(GrexFragment *, GrexTemplate, grex_template, fragment,
                 properties[PROP_FRAGMENT])

static void
on_resource_changed(GrexResourceLoader *loader, GResource *resource,
                    gpointer user_data) {
  GrexReactiveInflator *inflator = user_data;
  GrexTemplate *old_template = g_object_get_qdata(
      G_OBJECT(inflator), grex_inflator_original_template_quark());
  g_return_if_fail(old_template != NULL);

  g_autoptr(GBytes) new_bytes =
      g_resource_lookup_data(resource, old_template->resource_path,
                             G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (new_bytes != NULL) {
    g_message("Triggering reload for resource path '%s'",
              old_template->resource_path);

    g_autoptr(GrexTemplate) template = grex_template_new_from_resource(
        old_template->resource_path, old_template->scope, loader);
    g_return_if_fail(template != NULL);
    grex_reactive_inflator_change_fragment_and_inflate(inflator,
                                                       template->fragment);
  }
}

/**
 * grex_template_create_inflator:
 * @target: (transfer none): The target object.
 *
 * Creates a new reactive inflator that will inflate this template into the
 * given object.
 *
 * Returns: (transfer full): The new inflator.
 */
GrexReactiveInflator *
grex_template_create_inflator(GrexTemplate *template, GObject *target) {
  GrexReactiveInflator *inflator =
      grex_reactive_inflator_new(template->fragment, target);

  if (grex_resource_loader_is_reload_enabled(template->loader)) {
    g_object_set_qdata_full(G_OBJECT(inflator),
                            grex_inflator_original_template_quark(),
                            g_object_ref(template), g_object_unref);
    g_signal_connect_object(template->loader, "changed",
                            G_CALLBACK(on_resource_changed), inflator, 0);
  }

  return inflator;
}
