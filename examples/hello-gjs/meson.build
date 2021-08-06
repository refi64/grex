# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

gnome = import('gnome')

hello_gjs_resources = gnome.compile_resources(
  'hello-resources',
  'hello.gresource.xml',
  gresource_bundle: true,
)

# Copies the .js file to the build directory so it can find its resources.
custom_target(
  'hello-js',
  input : 'hello.js',
  output : 'hello.js',
  command : ['cp', '@INPUT@', '@OUTPUT@'],
  depends : [hello_gjs_resources, g_grex_lib, g_grex_typelib],
)
