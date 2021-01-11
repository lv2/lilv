autoship = find_program('autoship', required: false)

subdir('lv2/core.lv2')

subdir('bad_syntax.lv2')
subdir('failed_instantiation.lv2')
subdir('failed_lib_descriptor.lv2')
subdir('lib_descriptor.lv2')
subdir('missing_descriptor.lv2')
subdir('missing_name.lv2')
subdir('missing_plugin.lv2')
subdir('missing_port.lv2')
subdir('missing_port_name.lv2')
subdir('new_version.lv2')
subdir('old_version.lv2')
subdir('test_plugin.lv2')

unit_tests = [
  'bad_port_index',
  'bad_port_symbol',
  'classes',
  'discovery',
  'filesystem',
  'get_symbol',
  'no_author',
  'no_verify',
  'plugin',
  'port',
  'preset',
  'project',
  'project_no_author',
  'prototype',
  'reload_bundle',
  'replace_version',
  'state',
  'string',
  'ui',
  'util',
  'value',
  'verify',
  'world',
]

define_args = [
  '-DLILV_TEST_BUNDLE="@0@/"'.format(meson.current_build_dir() / 'test_plugin.lv2'),
  '-DLILV_TEST_DIR="@0@/"'.format(meson.current_build_dir()),
]

foreach unit : unit_tests
  test(unit,
       executable('test_@0@'.format(unit),
                  ['lilv_test_utils.c', 'test_@0@.c'.format(unit)],
                  c_args: prog_args + define_args,
                  include_directories: include_directories(['../src']),
                  dependencies: [lv2_dep, lilv_static_dep]),
       suite: 'unit')
endforeach

if autoship.found()
  test('autoship', autoship, args: ['test', lilv_src_root], suite: 'data')
endif
