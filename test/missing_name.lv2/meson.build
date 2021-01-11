module = shared_module('missing_name',
                       'missing_name.c',
                       dependencies: lv2_dep,
                       gnu_symbol_visibility: 'hidden',
                       name_prefix: '')

extension = '.' + module.full_path().split('.')[-1]
config = configuration_data({'SHLIB_EXT': extension})
ttl_files = ['manifest.ttl', 'missing_name.ttl']

foreach f : ttl_files
  configure_file(input: f + '.in',
                 output: f,
                 configuration: config)
endforeach

test('missing_name',
     executable('test_missing_name',
                'test_missing_name.c',
                c_args: prog_args,
                dependencies: [lv2_dep, lilv_static_dep]),
     args: [meson.current_build_dir() / ''],
     suite: 'plugin')

