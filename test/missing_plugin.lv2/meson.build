module = shared_module('missing_plugin',
                       'missing_plugin.c',
                       dependencies: lv2_dep,
                       gnu_symbol_visibility: 'hidden',
                       name_prefix: '')

extension = '.' + module.full_path().split('.')[-1]
config = configuration_data({'SHLIB_EXT': extension})
ttl_files = ['manifest.ttl', 'missing_plugin.ttl']

foreach f : ttl_files
  configure_file(input: f + '.in',
                 output: f,
                 configuration: config)
endforeach

test('missing_plugin',
     executable('test_missing_plugin',
                'test_missing_plugin.c',
                c_args: prog_args,
                dependencies: [lv2_dep, lilv_static_dep]),
     args: [meson.current_build_dir() / ''],
     suite: 'plugin')

