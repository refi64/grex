/* Any copyright is dedicated to the Public Domain.
 * https://creativecommons.org/publicdomain/zero/1.0/ */

#include "hello-window.h"

static GrexTemplate *template;

struct _HelloWindow {
  GtkApplicationWindow parent_instance;

  GrexInflator *inflator;
};

G_DEFINE_TYPE(HelloWindow, hello_window, GTK_TYPE_APPLICATION_WINDOW)

static void
hello_window_class_init(HelloWindowClass *klass) {
  template = grex_template_new_from_resource(
      "/org/hello/Hello/hello-window.xml", NULL);
  g_warn_if_fail(template != NULL);
}

static void
hello_window_init(HelloWindow *window) {
  window->inflator = grex_inflator_new();
  grex_template_inflate(template, window->inflator, GTK_WIDGET(window));
}

HelloWindow *
hello_window_new(GtkApplication *application) {
  return g_object_new(HELLO_TYPE_WINDOW, "application", application, NULL);
}
