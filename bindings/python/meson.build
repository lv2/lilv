# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

pymod = import('python')
py = pymod.find_installation('python3', required: get_option('bindings_py'))

if py.found()
  py.install_sources(files('lilv.py'))
endif
