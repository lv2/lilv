#!/usr/bin/env python
import autowaf
import Options

# Version of this package (even if built as a child)
SLV2_VERSION = '0.6.7'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Version history:
#   0.0.1 = 0,0,0
#   0.1.0 = 2,0,0
#   0.2.0 = 3,0,0
#   0.3.0 = 4,0,0
#   0.3.1 = 4,0,0
#   0.3.2 = 5,0,1
#   0.4.0 = 6,0,0
#   0.4.1 = 6,0,0 (oops, should have been 6,1,0)
#   0.4.2 = 6,0,0 (oops, should have been 6,2,0)
#   0.4.3 = 6,0,0 (oops, should have been 6,3,0)
#   0.4.4 = 7,0,1
#   0.4.5 = 7,0,1
#   0.5.0 = 8,0,0
#   0.6.0 = 9,0,0 (SVN r1282)
#   0.6.1 = 9,1,0
#   0.6.2 = 9,1,1
#   0.6.4 = 9,2,0
#   0.6.6 = 9,2,0
#   0.6.7 = 9,2,1
SLV2_LIB_VERSION = '9.2.1'

# Variables for 'waf dist'
APPNAME = 'slv2'
VERSION = SLV2_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
	autowaf.set_options(opt)
	opt.add_option('--no-jack', action='store_true', default=False, dest='no_jack',
			help="Do not build JACK clients")
	opt.add_option('--dyn-manifest', action='store_true', default=False, dest='dyn_manifest',
			help="Build support for dynamic manifest extension [false]")
	opt.add_option('--test', action='store_true', default=False, dest='build_tests',
			help="Build unit tests")
	opt.add_option('--bash-completion', action='store_true', default=False, dest='bash_completion',
			help="Install bash completion script in /etc/bash_completion.d")
	opt.add_option('--default-lv2-path', type='string', default='', dest='default_lv2_path',
			help="Default LV2 path to use if $LV2_PATH is unset (globs and ~ supported)")

def configure(conf):
	conf.line_just = max(conf.line_just, 59)
	autowaf.configure(conf)
	autowaf.display_header('SLV2 Configuration')
	conf.check_tool('compiler_cc')
	autowaf.check_pkg(conf, 'lv2core', uselib_store='LV2CORE', mandatory=True)
	autowaf.check_pkg(conf, 'redland', uselib_store='REDLAND', atleast_version='1.0.6', mandatory=True)
	autowaf.check_pkg(conf, 'jack', uselib_store='JACK', atleast_version='0.107.0', mandatory=False)
	conf.env.append_value('CFLAGS', '-std=c99')
	conf.define('SLV2_VERSION', SLV2_VERSION)
	if Options.options.dyn_manifest:
		conf.define('SLV2_DYN_MANIFEST', 1)

	if Options.options.default_lv2_path == '':
		if Options.platform == 'darwin':
			Options.options.default_lv2_path = "~/Library/Audio/Plug-Ins/LV2:/Library/Audio/Plug-Ins/LV2:~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2"
		elif Options.platform == 'haiku':
			Options.options.default_lv2_path = "~/.lv2:/boot/common/add-ons/lv2"
		else:
			Options.options.default_lv2_path = "~/.lv2:/usr/local/" + conf.env['LIBDIRNAME'] + '/lv2:' + conf.env['LIBDIR'] + '/lv2'

	conf.env['USE_JACK'] = conf.env['HAVE_JACK'] and not Options.options.no_jack
	conf.env['BUILD_TESTS'] = Options.options.build_tests
	conf.env['BASH_COMPLETION'] = Options.options.bash_completion
	conf.define('SLV2_DEFAULT_LV2_PATH', Options.options.default_lv2_path)

	if conf.env['USE_JACK']:
		autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event.h', 'HAVE_LV2_EVENT')
		autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-map/uri-map.h', 'HAVE_LV2_URI_MAP')
		conf.env['USE_JACK'] = conf.env['HAVE_LV2_EVENT'] and conf.env['HAVE_LV2_URI_MAP']

	conf.write_config_header('slv2-config.h', remove=False)

	autowaf.display_msg(conf, "Jack clients", str(conf.env['USE_JACK'] == 1))
	autowaf.display_msg(conf, "Unit tests", str(conf.env['BUILD_TESTS']))
	autowaf.display_msg(conf, "Dynamic Manifest Support", str(conf.env['SLV2_DYN_MANIFEST'] == 1))
	autowaf.display_msg(conf, "Default LV2_PATH", str(conf.env['SLV2_DEFAULT_LV2_PATH']))
	print

tests = '''
	test/slv2_test
'''

def build(bld):
	# C Headers
	bld.install_files('${INCLUDEDIR}/slv2', bld.path.ant_glob('slv2/*.h'))

	# Pkgconfig file
	autowaf.build_pc(bld, 'SLV2', SLV2_VERSION, ['REDLAND'])

	lib_source = '''
		src/collections.c
		src/plugin.c
		src/pluginclass.c
		src/plugininstance.c
		src/plugins.c
		src/pluginui.c
		src/pluginuiinstance.c
		src/port.c
		src/query.c
		src/scalepoint.c
		src/util.c
		src/value.c
		src/world.c
	'''

	# Library
	obj = bld(features = 'c cshlib')
	obj.export_includes = ['.']
	obj.source          = lib_source
	obj.includes        = ['.', './src']
	obj.name            = 'libslv2'
	obj.target          = 'slv2'
	obj.vnum            = SLV2_LIB_VERSION
	obj.install_path    = '${LIBDIR}'
	obj.cflags          = [ '-fvisibility=hidden', '-DSLV2_SHARED', '-DSLV2_INTERNAL' ]
	obj.ldflags         = [ '-ldl' ]
	autowaf.use_lib(bld, obj, 'REDLAND LV2CORE')

	if bld.env['BUILD_TESTS']:
		# Static library (for unit test code coverage)
		obj = bld(features = 'c cstaticlib')
		obj.source       = lib_source
		obj.includes     = ['.', './src']
		obj.name         = 'libslv2_static'
		obj.target       = 'slv2_static'
		obj.install_path = ''
		obj.cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ]
		autowaf.use_lib(bld, obj, 'REDLAND LV2CORE')

		# Unit tests
		for i in tests.split():
			obj = bld(features = 'c cprogram')
			obj.source       = i + '.c'
			obj.includes     = ['.', './src']
			obj.use          = 'libslv2_static'
			obj.uselib       = 'REDLAND LV2CORE'
			obj.libs         = 'gcov'
			obj.target       = i
			obj.install_path = ''
			obj.cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ]

	# Utilities
	if bld.env['BUILD_UTILS']:
		utils = '''
			utils/lv2_inspect
			utils/lv2_list
		'''
		for i in utils.split():
			obj = bld(features = 'c cprogram')
			obj.source       = i + '.c'
			obj.includes     = ['.', './src', './utils']
			obj.use          = 'libslv2'
			obj.target       = i
			obj.install_path = '${BINDIR}'

	# JACK Hosts
	hosts = 'hosts/lv2_jack_host'
	if bld.env['USE_JACK']:
		for i in hosts.split():
			obj = bld(features = 'c cprogram')
			obj.source       = i + '.c'
			obj.includes     = ['.', './src', './utils']
			obj.uselib       = 'JACK'
			obj.use          = 'libslv2'
			obj.target       = i
			obj.install_path = '${BINDIR}'

	# Documentation
	autowaf.build_dox(bld, 'SLV2', SLV2_VERSION, top, out)

	# Bash completion
	if bld.env['BASH_COMPLETION']:
		bld.install_as(
			'/etc/bash_completion.d/slv2', 'utils/slv2.bash_completion')

def test(ctx):
	autowaf.run_tests(ctx, APPNAME, tests.split())

def shutdown(self):
	autowaf.shutdown()
