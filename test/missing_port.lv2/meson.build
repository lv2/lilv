module = shared_module('missing_port',
                       'missing_port.c',
                       dependencies: lv2_dep,
                       gnu_symbol_visibility: 'hidden',
                       name_prefix: '')

extension = '.' + module.full_path().split('.')[-1]
config = configuration_data({'SHLIB_EXT': extension})
ttl_files = ['manifest.ttl', 'missing_port.ttl']

foreach f : ttl_files
  configure_file(input: f + '.in',
                 output: f,
                 configuration: config)
endforeach

test('missing_port',
     executable('test_missing_port',
                'test_missing_port.c',
                c_args: prog_args,
                dependencies: [lv2_dep, lilv_static_dep]),
     args: [meson.current_build_dir() / ''],
     suite: 'plugin')

