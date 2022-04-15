/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-template.h"

#include "gpropz.h"

struct _GrexTemplate {
  GObject parent_instance;

  GrexFragment *fragment;
};

enum {
  PROP_FRAGMENT = 1,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS] = {NULL};

G_DEFINE_TYPE(GrexTemplate, grex_template, G_TYPE_OBJECT)

static void
grex_template_dispose(GObject *object) {
  GrexTemplate *template = GREX_TEMPLATE(object);
  g_clear_object(&template->fragment);
}

static void
grex_template_class_init(GrexTemplateClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = grex_template_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_FRAGMENT] = g_param_spec_object(
      "fragment", "Fragment", "This template's fragment.", GREX_TYPE_FRAGMENT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexTemplate, fragment, PROP_FRAGMENT,
                          properties[PROP_FRAGMENT], NULL);
}

static void
grex_template_init(GrexTemplate *template) {}

/**
 * grex_template_new:
 * @fragment: (transfer none): The fragment to wrap in this template.
 *
 * Creates a new #GrexTemplate containing the given fragment.
 *
 * Returns: (transfer full): A new template.
 */
GrexTemplate *
grex_template_new(GrexFragment *fragment) {
  return g_object_new(GREX_TYPE_TEMPLATE, "fragment", fragment, NULL);
}

/**
 * grex_template_new_from_xml:
 * @xml: The XML content.
 * @len: Length of @xml in bytes, or -1 if null-terminated.
 * @filename: (nullable): The filename of the content, used in the resulting
 *                        fragment's source location.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names.
 *
 * Creates a new #GrexTemplate containing a fragment parsed from the given XML
 * data.
 *
 * Returns: (transfer full): A new template.
 */
GrexTemplate *
grex_template_new_from_xml(const char *xml, gssize len, const char *filename,
                           GtkBuilderScope *scope) {
  g_autoptr(GError) error = NULL;
  g_autoptr(GrexFragment) fragment =
      grex_fragment_parse_xml(xml, len, filename, scope, &error);
  if (fragment == NULL) {
    g_error("Failed to parse template (%s): %s",
            filename != NULL ? filename : "<unknown>", error->message);
    return NULL;
  }

  return grex_template_new(fragment);
}

/**
 * grex_template_new_from_resource:
 * @resource: The resource path.
 * @scope: (nullable): The #GtkBuilderScope to resolve type names.
 *
 * Creates a new #GrexTemplate containing a fragment parsed from the XML data
 * within the given resource path.
 *
 * Returns: (transfer full): A new template.
 */
GrexTemplate *
grex_template_new_from_resource(const char *resource, GtkBuilderScope *scope) {
  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) bytes =
      g_resources_lookup_data(resource, G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  if (bytes == NULL) {
    g_error("Failed to look up resource %s: %s", resource, error->message);
    return NULL;
  }

  gsize size = 0;
  const char *xml = g_bytes_get_data(bytes, &size);
  return grex_template_new_from_xml(xml, size, resource, scope);
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
  return grex_reactive_inflator_new(template->fragment, target);
}
