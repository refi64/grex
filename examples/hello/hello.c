/* Any copyright is dedicated to the Public Domain.
 * https://creativecommons.org/publicdomain/zero/1.0/ */

#include "hello-window.h"

#include <grex.h>

static void
app_activate(GApplication *app, gpointer user_data) {
  HelloWindow *window = hello_window_new(GTK_APPLICATION(app));
  gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char **argv) {
  g_autoptr(GtkApplication) app =
      gtk_application_new("org.hello.Hello", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}
