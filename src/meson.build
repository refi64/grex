# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

gnome = import('gnome')

# TODO: add stuff here
conf_data = configuration_data()

grex_config_h = configure_file(
  input : 'grex-config.in.h',
  output : 'grex-config.h',
  configuration : conf_data,
)

grex_sources = [
  'grex-binding.c',
  'grex-fragment.c',
  'grex-source-location.c',
]

grex_headers = [
  grex_config_h,
  'grex-binding.h',
  'grex-fragment.h',
  'grex-source-location.h',
]

grex_args = ['-D_GREX_INTERNAL']

g_grex_lib = library(
  'grex-@0@'.format(g_abi_version),
  grex_sources,
  c_args : grex_args,
  dependencies : g_grex_dependencies,
)

grex_dep = declare_dependency(
  include_directories : ['.'],
  link_with : [g_grex_lib],
)

grex_gir = gnome.generate_gir(
  g_grex_lib,
  sources : grex_sources + grex_headers,
  namespace : 'Grex',
  nsversion: g_abi_version.to_string(),
  identifier_prefix : 'Grex',
  symbol_prefix : 'grex',
  export_packages : 'grex-@0@'.format(g_abi_version),
  header : 'grex.h',
  includes : ['GObject-2.0', 'Gtk-4.0'],
  install : true,
  extra_args : grex_args,
)

g_grex_build_dir = meson.current_build_dir()
g_grex_typelib = grex_gir[0]
