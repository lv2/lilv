module = shared_module('new_version',
                       'new_version.c',
                       dependencies: lv2_dep,
                       gnu_symbol_visibility: 'hidden',
                       name_prefix: '')

extension = '.' + module.full_path().split('.')[-1]
config = configuration_data({'SHLIB_EXT': extension})
ttl_files = ['manifest.ttl', 'new_version.ttl']

foreach f : ttl_files
  configure_file(input: f + '.in',
                 output: f,
                 configuration: config)
endforeach

