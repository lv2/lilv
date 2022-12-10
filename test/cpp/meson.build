# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

test(
  'lilv_hpp',
  executable(
    'test_lilv_hpp',
    files('test_lilv_hpp.cpp'),
    c_args: test_args + c_suppressions,
    dependencies: [lv2_dep, lilv_static_dep],
    include_directories: include_directories('../../src'),
  ),
  suite: 'unit',
)
