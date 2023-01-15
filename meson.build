# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project('lilv', ['c'],
        version: '0.24.21',
        license: 'ISC',
        meson_version: '>= 0.56.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
        ])

lilv_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'lilv' + version_suffix

#######################
# Compilers and Flags #
#######################

# Required tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Enable C++ support if bindings aren't disabled
if not get_option('bindings_cpp').disabled()
  if add_languages(['cpp'], native: false, required: false)
    cpp = meson.get_compiler('cpp')
  endif
endif

# Set global warning flags
if get_option('strict') and not meson.is_subproject()
  subdir('meson/warnings')
endif
subdir('meson/suppressions')

##########################
# Platform Configuration #
##########################

platform_defines = ['-DLILV_VERSION="@0@"'.format(meson.project_version())]
if host_machine.system() in ['gnu', 'linux']
  platform_defines += [
    '-D_POSIX_C_SOURCE=200809L',
  ]
endif

default_lv2_path = get_option('default_lv2_path')
if default_lv2_path == ''
  if host_machine.system() == 'darwin'
    lv2_dirs = [
      '~/.lv2',
      '~/Library/Audio/Plug-Ins/LV2',
      '/usr/local/lib/lv2',
      '/usr/lib/lv2',
      '/Library/Audio/Plug-Ins/LV2',
    ]

    default_lv2_path = ':'.join(lv2_dirs)

  elif host_machine.system() == 'haiku'
    default_lv2_path = ':'.join(['~/.lv2', '/boot/common/add-ons/lv2'])

  elif host_machine.system() == 'windows'
    lv2_dirs = ['%%APPDATA%%\\\\LV2', '%%COMMONPROGRAMFILES%%\\\\LV2']
    default_lv2_path = ';'.join(lv2_dirs)

  else
    lv2_dirs = [
      '~' / '.lv2',
      '/usr/local' / get_option('libdir') / 'lv2',
      '/usr' / get_option('libdir') / 'lv2',
    ]

    default_lv2_path = ':'.join(lv2_dirs)
  endif
endif

platform_defines += [
  '-DLILV_DEFAULT_LV2_PATH="@0@"'.format(default_lv2_path)
]

# Use versioned name everywhere to support parallel major version installations
if host_machine.system() == 'windows'
  if get_option('default_library') == 'both'
    error('default_library=both is not supported on Windows')
  endif
  soversion = ''
else
  soversion = meson.project_version().split('.')[0]
endif

add_project_arguments(platform_defines, language: ['c'])

################
# Dependencies #
################

m_dep = cc.find_library('m', required: false)
dl_dep = cc.find_library('dl', required: false)

zix_dep = dependency('zix-0', fallback: 'zix', version: '>= 0.3.0')
serd_dep = dependency('serd-0', fallback: 'serd', version: '>= 0.30.10')
sord_dep = dependency('sord-0', fallback: 'sord', version: '>= 0.16.10')
lv2_dep = dependency('lv2', fallback: 'lv2', version: '>= 1.18.2')
sratom_dep = dependency('sratom-0', fallback: 'sratom', version: '>= 0.6.10')

###########
# Library #
###########

c_headers = files('include/lilv/lilv.h')
cpp_headers = files('include/lilv/lilvmm.hpp')

sources = files(
  'src/collections.c',
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
)

common_dependencies = [
  dl_dep,
  lv2_dep,
  m_dep,
  serd_dep,
  sord_dep,
  sratom_dep,
  zix_dep,
]

# Set appropriate arguments for building against the library type
extra_c_args = []
if get_option('default_library') == 'static'
  extra_c_args = ['-DLILV_STATIC']
endif

# Build main shared and/or static library
liblilv = library(
  versioned_name,
  sources,
  c_args: c_suppressions +  extra_c_args + ['-DLILV_INTERNAL'],
  dependencies: common_dependencies,
  gnu_symbol_visibility: 'hidden',
  include_directories: include_directories('include', 'src'),
  install: true,
  soversion: soversion,
  version: meson.project_version(),
)

# Declare dependency for internal meson dependants
lilv_dep = declare_dependency(
  compile_args: extra_c_args,
  dependencies: common_dependencies,
  include_directories: include_directories('include'),
  link_with: liblilv,
)

# Generage pkg-config file for external dependants
pkg.generate(
  liblilv,
  description: 'Library for hosting LV2 plugins',
  extra_cflags: extra_c_args,
  filebase: versioned_name,
  name: 'Lilv',
  requires: ['lv2'],
  subdirs: [versioned_name],
  version: meson.project_version(),
)

# Override pkg-config dependency for internal meson dependants
meson.override_dependency(versioned_name, lilv_dep)

# Install headers to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'lilv')
install_headers(cpp_headers, subdir: versioned_name / 'lilv')

#########
# Tools #
#########

if not get_option('tools').disabled()
  subdir('tools')
endif

############
# Bindings #
############

if not get_option('bindings_py').disabled()
  subdir('bindings/python')
endif

###########
# Support #
###########

if not get_option('docs').disabled()
  subdir('doc')
endif

if not get_option('tests').disabled()
  # Get or build a static library for linking internal tests
  if get_option('default_library') == 'both'
    liblilv_static = liblilv.get_static_lib()
  elif get_option('default_library') == 'shared'
    liblilv_static = static_library(
      versioned_name,
      sources,
      include_directories: include_directories('include', 'src'),
      c_args: c_suppressions + ['-DLILV_INTERNAL', '-DLILV_STATIC'],
      dependencies: common_dependencies,
      gnu_symbol_visibility: 'default')
  else
    liblilv_static = liblilv
  endif

  lilv_static_dep = declare_dependency(
    compile_args: extra_c_args,
    dependencies: common_dependencies,
    include_directories: include_directories('include'),
    link_with: liblilv_static,
  )

  # Build and run tests against static library
  subdir('test')
endif

if not meson.is_subproject()
  summary('Tests', not get_option('tests').disabled(), bool_yn: true)
  summary('Tools', not get_option('tools').disabled(), bool_yn: true)

  summary('Install prefix', get_option('prefix'))
  summary('Default LV2_PATH', default_lv2_path)

  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))

  if not get_option('tools').disabled()
    summary('Executables', get_option('prefix') / get_option('bindir'))
    summary('Man pages', get_option('prefix') / get_option('mandir'))
  endif
endif
