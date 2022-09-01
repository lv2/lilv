# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

ttl_files = files(
  'lv2core.ttl',
  'manifest.ttl',
)

foreach file : ttl_files
  configure_file(copy: true, input: file, output: '@PLAINNAME@')
endforeach
