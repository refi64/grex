/* Any copyright is dedicated to the Public Domain.
 * https://creativecommons.org/publicdomain/zero/1.0/ */

#include "hello-window.h"

#include "gpropz.h"

static GrexTemplate *template;

struct _HelloWindow {
  GtkApplicationWindow parent_instance;

  int elapsed;
  gboolean timer_visible;
  guint timer_source;

  GrexReactiveInflator *inflator;
};

G_DEFINE_TYPE(HelloWindow, hello_window, GTK_TYPE_APPLICATION_WINDOW)

enum {
  PROP_ELAPSED = 1,
  PROP_TIMER_VISIBLE,
  N_PROPS,
};

enum {
  SIGNAL_RESET = 1,
  N_SIGNALS,
};

static GParamSpec *properties[N_PROPS] = {NULL};
static guint signals[N_SIGNALS] = {0};

static void
hello_window_dispose(GObject *object) {
  HelloWindow *window = HELLO_WINDOW(object);
  g_clear_object(&window->inflator);

  if (window->timer_source != 0) {
    GSource *source = g_main_context_find_source_by_id(g_main_context_default(),
                                                       window->timer_source);
    if (source != NULL) {
      g_source_destroy(source);
    }

    window->timer_source = 0;
  }
}

static void
hello_window_reset(HelloWindow *window, GtkButton *button) {
  g_object_set(window, "elapsed", 0, NULL);
  g_print("button: %p\n", button);
}

static void
hello_window_class_init(HelloWindowClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = hello_window_dispose;

  gpropz_class_init_property_functions(object_class);

  properties[PROP_ELAPSED] =
      g_param_spec_int("elapsed", "Elapsed seconds",
                       "Seconds elapsed since the application start.", 0,
                       G_MAXINT, 0, G_PARAM_READWRITE);
  gpropz_install_property(object_class, HelloWindow, elapsed, PROP_ELAPSED,
                          properties[PROP_ELAPSED], NULL);

  properties[PROP_TIMER_VISIBLE] = g_param_spec_boolean(
      "timer-visible", "Timer visible",
      "Determines if the timer is currently visible.", TRUE, G_PARAM_READWRITE);
  gpropz_install_property(object_class, HelloWindow, timer_visible,
                          PROP_TIMER_VISIBLE, properties[PROP_TIMER_VISIBLE],
                          NULL);

  signals[SIGNAL_RESET] =
      g_signal_new("reset", G_TYPE_FROM_CLASS(object_class),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 1, GTK_TYPE_BUTTON);

  template = grex_template_new_from_resource(
      "/org/hello/Hello/hello-window.xml", NULL);
  g_warn_if_fail(template != NULL);
}

static int
on_second(gpointer user_data) {
  HelloWindow *window = HELLO_WINDOW(user_data);

  window->elapsed += 1;
  g_object_notify_by_pspec(G_OBJECT(window), properties[PROP_ELAPSED]);

  return G_SOURCE_CONTINUE;
}

static void
hello_window_init(HelloWindow *window) {
  window->timer_visible = TRUE;

  window->inflator = grex_template_create_inflator(template, G_OBJECT(window));
  grex_inflator_take_directives(
      grex_reactive_inflator_get_base_inflator(window->inflator),
      GREX_INFLATOR_DIRECTIVE_NONE, grex_if_directive_factory_new(),
      grex_gtk_child_property_container_directive_factory_new(),
      grex_gtk_box_container_directive_factory_new(), NULL);

  grex_reactive_inflator_inflate(window->inflator);

  window->timer_source = g_timeout_add_seconds(1, on_second, window);

  g_signal_connect(window, "reset", G_CALLBACK(hello_window_reset), NULL);
}

HelloWindow *
hello_window_new(GtkApplication *application) {
  return g_object_new(HELLO_TYPE_WINDOW, "application", application, NULL);
}
