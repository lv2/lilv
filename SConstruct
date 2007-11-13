#!python (This script must be run with scons)
# See INSTALL for build instructions, and COPYING for licensing information.

import os

print "WARNING: SCons building is experimental"
print "WARNING: This should NOT be used to build a system-installed SLV2"
print "WARNING: Use ./configure; make; make install"

universal_cflags = '-std=c99 -I. -Wextra'
debug_cflags = '-O0 -g -DDEBUG'
opt_cflags = '-O2 -fomit-frame-pointer -DNDEBUG'
sys_cflags = os.environ['CFLAGS']

env = Environment(ENV = {'PATH' : os.environ['PATH'] })
env.SConsignFile()

opt = Options(['options.cache'])
opt.AddOptions(
	BoolOption('JACK', 'Build JACK clients', True),
	BoolOption('DEBUG', 'Debug build', False))
opt.Update(env)
opt.Save('options.cache',env)
Help(opt.GenerateHelpText(env))

configure = env.Configure()

env.ParseConfig('pkg-config --cflags --libs jack')
env.ParseConfig('redland-config --cflags --libs')

env.Append(CCFLAGS = universal_cflags)
if env['DEBUG']:
	print "Using debug CFLAGS:\t", debug_cflags
	env.Append(CCFLAGS = debug_cflags)
elif sys_cflags:
	print "Using system CFLAGS:\t", sys_cflags
	env.Append(CCFLAGS = sys_cflags)
else:
	print "Using default CFLAGS:\t", opt_cflags
	env.Append(CCFLAGS = opt_cflags)

env.Append(CCFLAGS = "-DCONFIG_H_PATH=\\\"" + os.path.abspath(".") + "/config/config.h\\\"")

slv2_sources = Split('''
src/plugin.c
src/pluginclass.c
src/pluginclasses.c
src/plugininstance.c
src/plugins.c
src/pluginui.c
src/pluginuis.c
src/pluginuiinstance.c
src/port.c
src/query.c
src/util.c
src/value.c
src/values.c
src/world.c
''')


env.SharedLibrary('slv2', slv2_sources)

# Build against the local version we just built
env.Prepend(LIBS = 'slv2', LIBPATH = '.')

env.Program('lv2_inspect', [ 'utils/lv2_inspect.c' ])
env.Append(CCFLAGS = '-std=c99 -I.')
env.Program('lv2_list', [ 'utils/lv2_list.c' ])
env.Program('ladspa2lv2', [ 'utils/ladspa2lv2.c' ])

if env['JACK']:
	env.Program('lv2_simple_jack_host', [ 'hosts/lv2_simple_jack_host.c' ])
	env.Program('lv2_jack_host', [ 'hosts/lv2_jack_host.c' ])
