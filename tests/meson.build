# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

test_env = environment()
test_env.prepend('LD_LIBRARY_PATH', g_grex_build_dir)
test_env.prepend('GI_TYPELIB_PATH', g_grex_build_dir)

pytest_args = []
if get_option('pytest-force-colors')
  pytest_args += ['--color=yes']
endif

test(
  'introspection',
  g_python,
  args : ['-m', 'pytest', meson.current_source_dir()] + pytest_args,
  env : test_env,
  depends : [g_grex_lib, g_grex_typelib],
)
