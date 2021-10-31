# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

project('grex', 'c',
  version : '0.1.0',
  meson_version : '>= 0.56',
)

gpropz_proj = subproject('gpropz')
gpropz_dep = gpropz_proj.get_variable('gpropz_dep')

packcc_proj = subproject('packcc')
g_packcc = packcc_proj.get_variable('packcc')

pymod = import('python')
g_python = pymod.find_installation(
  'python3',
  modules : ['pytest'],
  required : true
)

g_abi_version = 1

g_grex_dependencies = [
  dependency('glib-2.0', version : '>= 2.58'),
  dependency('gtk4', version : '>= 4.0'),
  gpropz_dep,
]

subdir('src')
subdir('examples')
subdir('tests')
