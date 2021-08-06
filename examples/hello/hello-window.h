/* Any copyright is dedicated to the Public Domain.
 * https://creativecommons.org/publicdomain/zero/1.0/ */

#pragma once

#include <grex.h>

#define HELLO_TYPE_WINDOW hello_window_get_type()
G_DECLARE_FINAL_TYPE(HelloWindow, hello_window, HELLO, WINDOW,
                     GtkApplicationWindow)

HelloWindow *hello_window_new(GtkApplication *application);
