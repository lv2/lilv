module = shared_module('failed_lib_descriptor',
                       'failed_lib_descriptor.c',
                       dependencies: lv2_dep,
                       gnu_symbol_visibility: 'hidden',
                       name_prefix: '')

extension = '.' + module.full_path().split('.')[-1]
config = configuration_data({'SHLIB_EXT': extension})
ttl_files = ['manifest.ttl', 'failed_lib_descriptor.ttl']

foreach f : ttl_files
  configure_file(input: f + '.in',
                 output: f,
                 configuration: config)
endforeach

test('failed_lib_descriptor',
     executable('test_failed_lib_descriptor',
                'test_failed_lib_descriptor.c',
                c_args: prog_args,
                dependencies: [lv2_dep, lilv_static_dep]),
     args: [meson.current_build_dir() / ''],
     suite: 'plugin')

