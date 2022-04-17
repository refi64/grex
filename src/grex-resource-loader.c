/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "grex-resource-loader.h"

#include "gpropz.h"

struct _GrexResourceLoader {
  GObject parent_instance;

  gboolean reload_enabled;
};

G_DEFINE_FINAL_TYPE(GrexResourceLoader, grex_resource_loader, G_TYPE_OBJECT)

enum { PROP_IS_RELOAD_ENABLED = 1, N_PROPS };

static GParamSpec *properties[N_PROPS] = {NULL};

enum { SIGNAL_CHANGED = 1, N_SIGNALS };

static guint signals[N_SIGNALS] = {0};

static void
grex_resource_loader_class_init(GrexResourceLoaderClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  gpropz_class_init_property_functions(object_class);

  properties[PROP_IS_RELOAD_ENABLED] =
      g_param_spec_boolean("is-reload-enabled", "Is reloading enabled",
                           "Is monitoring resources to reload them enabled?",
                           FALSE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  gpropz_install_property(object_class, GrexResourceLoader, reload_enabled,
                          PROP_IS_RELOAD_ENABLED,
                          properties[PROP_IS_RELOAD_ENABLED], NULL);

  signals[SIGNAL_CHANGED] =
      g_signal_new("changed", G_TYPE_FROM_CLASS(object_class),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_RESOURCE);
}

static void
grex_resource_loader_init(GrexResourceLoader *loader) {}

typedef struct {
  GrexResourceLoader *loader;
  GResource *resource;
} ResourceChangeData;

static void
on_resource_changed(GFileMonitor *monitor, GFile *file, GFile *other_file,
                    GFileMonitorEvent event_type, gpointer user_data) {
  ResourceChangeData *data = user_data;
  g_autoptr(GError) error = NULL;

  if (event_type != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
    return;
  }

  g_autoptr(GResource) new_resource =
      g_resource_load(g_file_peek_path(file), &error);
  if (new_resource == NULL) {
    g_warning("Failed to reload resource '%s': %s", g_file_peek_path(file),
              error->message);
    return;
  }

  g_autoptr(GResource) previous_resource = g_steal_pointer(&data->resource);
  g_resources_unregister(previous_resource);

  g_resources_register(new_resource);
  data->resource = g_steal_pointer(&new_resource);

  g_signal_emit(data->loader, signals[SIGNAL_CHANGED], 0, data->resource);
}

/**
 * grex_resource_loader_default:
 *
 * Retrieves the global resource loader.
 *
 * Returns: (transfer none): The global resource loader.
 */
GrexResourceLoader *
grex_resource_loader_default() {
  static GrexResourceLoader *loader = NULL;
  if (g_once_init_enter(&loader)) {
    gboolean reload_enabled = g_strcmp0(g_getenv("GREX_RELOAD"), "1") == 0;
    if (reload_enabled) {
      g_message("Hot reloading support enabled!");
    }

    g_once_init_leave(&loader, grex_resource_loader_new(reload_enabled));
  }

  return loader;
}

/**
 * grex_resource_loader_new:
 * @is_reload_enabled: %TRUE if hot reload support should be enabled.
 *
 * Creates a new resource loader.
 *
 * Returns: (transfer full): A new #GrexResourceLoader.
 */
GrexResourceLoader *
grex_resource_loader_new(gboolean is_reload_enabled) {
  return g_object_new(GREX_TYPE_RESOURCE_LOADER, "is-reload-enabled",
                      is_reload_enabled, NULL);
}

/**
 * grex_resource_loader_is_reload_enabled:
 *
 * Checks if "hot reload" support is enabled (GREX_RELOAD=1 was set).
 *
 * Returns: %TRUE if hot reload support is enabled.
 */
gboolean
grex_resource_loader_is_reload_enabled(GrexResourceLoader *loader) {
  return loader->reload_enabled;
}

/**
 * grex_resource_loader_register:
 * @filename: The path to load from.
 * @error: Return location for a #GError.
 *
 * Loads and registers the resource at the given path.
 *
 * Returns: %TRUE if the resource was loaded successfully.
 */
gboolean
grex_resource_loader_register(GrexResourceLoader *loader, const char *filename,
                              GError **error) {
  g_autoptr(GFile) file = g_file_new_for_path(filename);

  g_autoptr(GResource) resource = g_resource_load(filename, error);
  if (resource == NULL) {
    g_prefix_error(error, "Failed to load resource '%s': ", filename);
    return FALSE;
  }

  if (loader->reload_enabled) {
    g_autoptr(GFileMonitor) monitor =
        g_file_monitor(file, G_FILE_MONITOR_WATCH_HARD_LINKS, NULL, error);
    if (monitor == NULL) {
      g_prefix_error(error, "Failed to monitor resource '%s': ", filename);
      return FALSE;
    }

    ResourceChangeData *data = g_new0(ResourceChangeData, 1);
    data->loader = g_object_ref(loader);
    data->resource = g_resource_ref(resource);

    g_signal_connect(monitor, "changed", G_CALLBACK(on_resource_changed), data);
    // Let it stick around forever.
    g_steal_pointer(&monitor);

    g_info("Hot reloading enabled for resource '%s'", filename);
  }

  g_resources_register(resource);
  return TRUE;
}
