# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

include_dirs = include_directories('../src')

#############################
# "Basic" (Lilv-Only) Tools #
#############################

basic_tools = [
  'lv2info',
  'lv2ls',
]

foreach tool : basic_tools
  executable(
    tool,
    files(tool + '.c'),
    c_args: c_suppressions,
    dependencies: lilv_dep,
    include_directories: include_dirs,
    install: true,
  )

  install_man(files('..' / 'doc' / tool + '.1'))
endforeach

install_data(
  files('lilv.bash_completion'),
  install_dir: get_option('sysconfdir') / 'bash_completion.d',
  rename: 'lilv',
)

###########################
# lv2apply (uses sndfile) #
###########################

sndfile_dep = dependency(
  'sndfile',
  version: '>= 1.0.0',
  required: get_option('tools'),
)

if sndfile_dep.found()
  executable(
    'lv2apply',
    files('lv2apply.c'),
    c_args: c_suppressions,
    dependencies: [lilv_dep, sndfile_dep],
    include_directories: include_dirs,
    install: true,
  )

  install_man(files('..' / 'doc' / 'lv2apply.1'))
endif

#################################
# lv2bench (uses clock_gettime) #
#################################

if host_machine.system() != 'windows'
  rt_dep = cc.find_library('rt', required: false)

  clock_gettime_code = '''#include <time.h>
int main(void) { struct timespec t; return clock_gettime(CLOCK_MONOTONIC, &t); }
'''

  if cc.compiles(clock_gettime_code,
                 args: platform_defines,
                 dependencies: [rt_dep],
                 name: 'clock_gettime')
    executable(
      'lv2bench',
      files('lv2bench.c'),
      c_args: c_suppressions,
      dependencies: [lilv_dep, rt_dep, sndfile_dep],
      include_directories: include_dirs,
      install: true,
    )

    install_man(files('..' / 'doc' / 'lv2bench.1'))
  endif
endif
