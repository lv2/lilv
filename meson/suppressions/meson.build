# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions

warning_level = get_option('warning_level')

#####
# C #
#####

c_suppressions = []

if cc.get_id() in ['clang', 'emscripten']
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-declaration-after-statement',
      '-Wno-documentation-unknown-command',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-format-nonliteral',
      '-Wno-implicit-float-conversion',
      '-Wno-implicit-int-conversion',
      '-Wno-nullable-to-nonnull-conversion',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-shorten-64-to-32',
      '-Wno-sign-conversion',
      '-Wno-switch-enum',
      '-Wno-vla',
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '-Wno-nullability-extension',
    ]

    if host_machine.system() == 'freebsd'
      c_suppressions += [
        '-Wno-c11-extensions',
      ]
    elif host_machine.system() == 'darwin'
      c_suppressions += [
        '-Wno-unused-macros',
      ]
    elif host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-deprecated-declarations',
        '-Wno-nonportable-system-include-path',
        '-Wno-unused-macros',
      ]
    endif
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
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
      '-Wno-unused-parameter',
      '-Wno-vla',
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '-Wno-pedantic',
    ]
  endif

  if host_machine.system() == 'windows'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-unused-macros',
    ]
  endif

elif cc.get_id() == 'msvc'
  c_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    c_suppressions += [
      '/wd4061', # enumerator in switch is not explicitly handled
      '/wd4191', # unsafe conversion from FARPROC
      '/wd4365', # signed/unsigned mismatch
      '/wd4514', # unreferenced inline function has been removed
      '/wd4710', # function not inlined
      '/wd4711', # function selected for automatic inline expansion
      '/wd4774', # format string is not a string literal
      '/wd4800', # implicit conversion to bool
      '/wd4820', # padding added after construct
      '/wd5045', # will insert Spectre mitigation for memory load
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '/wd4090', # different const qualifiers
      '/wd4706', # assignment within conditional expression
    ]
  endif

  if warning_level in ['everything', '3', '2']
    c_suppressions += [
      '/wd4244', # conversion from floating point, possible loss of data
      '/wd4267', # conversion from size_t, possible loss of data
      '/wd4996', # POSIX name for this item is deprecated
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)
