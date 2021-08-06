# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

gnome = import('gnome')
resources = gnome.compile_resources(
  'hello-resources',
  'hello.gresource.xml',
)

executable(
  'hello',
  [
    'hello-window.c',
    'hello.c',
  ] + resources,
  dependencies : [g_grex_dep],
)
