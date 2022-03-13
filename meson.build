project('lilv', ['c'],
        version: '1.0.0',
        license: 'ISC',
        meson_version: '>= 0.49.2',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
          'default_library=shared',
          'warning_level=2',
        ])

lilv_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'lilv' + version_suffix

# Load build tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Set ultra strict warnings for developers, if requested
if get_option('strict')
  subdir('meson')

  c_warnings = all_c_warnings
  if cc.get_id() == 'clang'
    c_warnings += [
      '-Wno-c11-extensions',
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-documentation-unknown-command',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-format-nonliteral',
      '-Wno-implicit-float-conversion',
      '-Wno-implicit-int-conversion',
      '-Wno-nullability-extension',
      '-Wno-nullable-to-nonnull-conversion',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-shorten-64-to-32',
      '-Wno-sign-conversion',
      '-Wno-switch-enum',
      '-Wno-unused-macros',
      '-Wno-vla',
    ]
  elif cc.get_id() == 'gcc'
    c_warnings += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-conversion',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-format-nonliteral',
      '-Wno-format-truncation',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-stack-protector',
      '-Wno-strict-overflow',
      '-Wno-suggest-attribute=const',
      '-Wno-suggest-attribute=pure',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsuffixed-float-constants',
      '-Wno-unused-const-variable',
      '-Wno-unused-macros',
      '-Wno-unused-parameter',
      '-Wno-vla',
    ]
  elif cc.get_id() == 'msvc'
    c_warnings += [
      '/wd4061',  # enumerator in switch is not explicitly handled
      '/wd4365',  # signed/unsigned mismatch
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4706',  # assignment within conditional expression
      '/wd4774',  # format string is not a string literal
      '/wd4820',  # padding added after construct
      '/wd4996',  # POSIX name for this item is deprecated
      '/wd5045',  # compiler will insert Spectre mitigation
    ]
  endif

  add_project_arguments(cc.get_supported_arguments(c_warnings),
                        language: ['c'])
endif

# Add special arguments for MSVC
if cc.get_id() == 'msvc'
  msvc_args = [
    '/D_CRT_SECURE_NO_WARNINGS',
    '/TP',
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  add_project_arguments(msvc_args, language: ['c'])
endif

c_headers = ['include/lilv/lilv.h']
c_header_files = files(c_headers)

sources = [
  'src/collections.c',
  'src/filesystem.c',
  'src/instance.c',
  'src/lib.c',
  'src/node.c',
  'src/plugin.c',
  'src/pluginclass.c',
  'src/port.c',
  'src/query.c',
  'src/scalepoint.c',
  'src/state.c',
  'src/ui.c',
  'src/util.c',
  'src/world.c',
  'src/zix/tree.c',
]

# System libraries
m_dep = cc.find_library('m', required: false)
dl_dep = cc.find_library('dl', required: false)

# Dependencies

lv2_dep = dependency('lv2',
                     version: '>= 1.18.2',
                     fallback: ['lv2', 'lv2_dep'])

serd_dep = dependency('serd-1',
                      version: '>= 1.0.0',
                      fallback: ['serd', 'serd_dep'])

sratom_dep = dependency('sratom-1',
                        version: '>=1.0.0',
                        fallback: ['sratom', 'sratom_dep'])

# Determine library type and the flags needed to build it
if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif

  library_type = 'both_libraries'
  library_args = ['-DLILV_INTERNAL']
  prog_args = []
elif get_option('default_library') == 'shared'
  library_type = 'shared_library'
  library_args = ['-DLILV_INTERNAL']
  prog_args = []
else
  library_type = 'static_library'
  library_args = ['-DLILV_INTERNAL', '-DLILV_STATIC']
  prog_args = ['-DLILV_STATIC']
endif

library_args += ['-DZIX_STATIC']

# Build shared and/or static library/libraries
liblilv = build_target(
  versioned_name,
  sources,
  version: meson.project_version(),
  include_directories: include_directories(['include', 'src']),
  c_args: library_args,
  dependencies: [m_dep, dl_dep, lv2_dep, serd_dep, sratom_dep],
  gnu_symbol_visibility: 'hidden',
  install: true,
  target_type: library_type)

lilv_dep = declare_dependency(
  include_directories: include_directories(['include']),
  dependencies: [m_dep, dl_dep, lv2_dep, serd_dep, sratom_dep],
  link_with: liblilv)

pkg.generate(
  liblilv,
  name: 'Lilv',
  filebase: versioned_name,
  subdirs: [versioned_name],
  version: meson.project_version(),
  description: 'A library for hosting LV2 plugins')

# Build lilvi command line utility
# if get_option('utils')

#   lilvi = executable('lilvi', 'src/lilvi.c',
#                      c_args: prog_args,
#                      install: true,
#                      dependencies: lilv_dep)

#   if not get_option('docs').disabled()
#     install_man('doc/lilvi.1')
#   endif

# endif

# Install header to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'lilv')

# if not get_option('docs').disabled()
#   subdir('doc')
# endif

if get_option('tests')
  if library_type == 'both_libraries'
    liblilv_static = liblilv.get_static_lib()
  elif library_type == 'shared_library'
    liblilv_static = static_library(
      versioned_name,
      sources,
      include_directories: include_directories(['include', 'src']),
      c_args: library_args,
      dependencies: [m_dep, dl_dep, lv2_dep, serd_dep, sratom_dep],
      gnu_symbol_visibility: 'default')
  else
    liblilv_static = liblilv
  endif

  lilv_static_dep = declare_dependency(
    include_directories: include_directories(['include']),
    dependencies: [m_dep, dl_dep, lv2_dep, serd_dep, sratom_dep],
    link_with: liblilv_static)

  subdir('test')
endif

if not meson.is_subproject() and meson.version().version_compare('>=0.53.0')
  summary('Tests', get_option('tests'), bool_yn: true)
  summary('Utilities', get_option('utils'), bool_yn: true)

  summary('Install prefix', get_option('prefix'))

  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))

  if get_option('utils')
    summary('Executables', get_option('prefix') / get_option('bindir'))
    summary('Man pages', get_option('prefix') / get_option('mandir'))
  endif
endif
