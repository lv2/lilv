#!/usr/bin/env python
import autowaf
import os
import sys
import Options

# Version of this package (even if built as a child)
SLV2_VERSION = '0.7.0alpha'

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
SLV2_LIB_VERSION = '10.0.0'

# Variables for 'waf dist'
APPNAME = 'slv2'
VERSION = SLV2_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
	autowaf.set_options(opt)
	opt.add_option('--no-utils', action='store_true', default=False, dest='no_utils',
			help="Do not build command line utilities")
	opt.add_option('--no-jack', action='store_true', default=False, dest='no_jack',
			help="Do not build JACK clients")
	opt.add_option('--no-jack-session', action='store_true', default=False,
			dest='no_jack_session',
			help="Do not build JACK session support")
	opt.add_option('--no-swig', action='store_true', default=False, dest='no_swig',
			help="Do not build python bindings")
	opt.add_option('--no-dyn-manifest', action='store_true', default=False,
			dest='no_dyn_manifest',
			help="Don't build support for dynamic manifests")
	opt.add_option('--test', action='store_true', default=False, dest='build_tests',
			help="Build unit tests")
	opt.add_option('--no-bash-completion', action='store_true', default=False,
			dest='no_bash_completion',
			help="Install bash completion script in /etc/bash_completion.d")
	opt.add_option('--default-lv2-path', type='string', default='',
			dest='default_lv2_path',
			help="Default LV2 path to use if $LV2_PATH is unset (globs and ~ supported)")

def configure(conf):
	conf.line_just = max(conf.line_just, 59)
	autowaf.configure(conf)
	autowaf.display_header('SLV2 Configuration')
	conf.check_tool('compiler_cc')

	if not Options.options.no_swig:
		try:
			conf.load('swig python')
			conf.check_python_headers()
			autowaf.define(conf, 'SLV2_SWIG', 1);
		except:
			pass

	autowaf.check_pkg(conf, 'lv2core', uselib_store='LV2CORE', mandatory=True)
	autowaf.check_pkg(conf, 'glib-2.0', uselib_store='GLIB',
	                  atleast_version='2.0.0', mandatory=True)
	autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
	                  atleast_version='2.0.0', mandatory=False)
	autowaf.check_pkg(conf, 'serd', uselib_store='SERD',
	                  atleast_version='0.1.0', mandatory=True)
	autowaf.check_pkg(conf, 'sord', uselib_store='SORD',
	                  atleast_version='0.1.0', mandatory=True)
	autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
	                  atleast_version='0.107.0', mandatory=False)
	autowaf.check_pkg(conf, 'jack', uselib_store='NEW_JACK',
					  atleast_version='0.120.0', mandatory=False)

	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/lv2core/lv2.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/extensions/ui/ui.h',
						 'HAVE_UI_H')

	if conf.env['HAVE_UI_H']:
		autowaf.define(conf, 'SLV2_WITH_UI', 1)

	if not Options.options.no_jack_session:
		if conf.env['HAVE_NEW_JACK'] and conf.env['HAVE_GTHREAD']:
			autowaf.define(conf, 'SLV2_JACK_SESSION', 1)
			
	conf.env.append_value('CFLAGS', '-std=c99')
	autowaf.define(conf, 'SLV2_VERSION', SLV2_VERSION)
	if not Options.options.no_dyn_manifest:
		autowaf.define(conf, 'SLV2_DYN_MANIFEST', 1)

	slv2_path_sep = ':'
	slv2_dir_sep  = '/'
	if sys.platform == 'win32':
		slv2_path_sep = ';'
		slv2_dir_sep = '\\'

	autowaf.define(conf, 'SLV2_PATH_SEP', slv2_path_sep)
	autowaf.define(conf, 'SLV2_DIR_SEP',  slv2_dir_sep)

	if Options.options.default_lv2_path == '':
		if Options.platform == 'darwin':
			Options.options.default_lv2_path = slv2_path_sep.join([
					'~/Library/Audio/Plug-Ins/LV2',
					'~/.lv2',
					'/usr/local/lib/lv2',
					'/usr/lib/lv2',
					'/Library/Audio/Plug-Ins/LV2'])
		elif Options.platform == 'haiku':
			Options.options.default_lv2_path = slv2_path_sep.join([
				'~/.lv2',
				'/boot/common/add-ons/lv2'])
		elif Options.platform == 'win32':
			Options.options.default_lv2_path = 'C:\\Program Files\\LV2'
		else:
			Options.options.default_lv2_path = slv2_path_sep.join([
					'~/.lv2',
					'/usr/%s/lv2' % conf.env['LIBDIRNAME'],
					'/usr/local/%s/lv2' % conf.env['LIBDIRNAME']])

	autowaf.define(conf, 'SLV2_DEFAULT_LV2_PATH', Options.options.default_lv2_path)

	conf.env['BUILD_TESTS']     = Options.options.build_tests
	conf.env['BUILD_UTILS']     = not Options.options.no_utils
	conf.env['BASH_COMPLETION'] = not Options.options.no_bash_completion

	conf.env['USE_JACK'] = conf.env['HAVE_JACK'] and not Options.options.no_jack
	if conf.env['USE_JACK']:
		autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event.h',
							 'HAVE_LV2_EVENT')
		autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-map/uri-map.h',
							 'HAVE_LV2_URI_MAP')
		if not (conf.env['HAVE_LV2_EVENT'] and conf.env['HAVE_LV2_URI_MAP']):
			conf.env['USE_JACK'] = False

	conf.write_config_header('slv2-config.h', remove=False)

	autowaf.display_msg(conf, "Default LV2_PATH",
	                    conf.env['SLV2_DEFAULT_LV2_PATH'])
	autowaf.display_msg(conf, "Utilities",
	                    bool(conf.env['BUILD_UTILS']))
	autowaf.display_msg(conf, "Jack clients",
	                    bool(conf.env['USE_JACK']))
	autowaf.display_msg(conf, "Jack session support",
	                    bool(conf.env['SLV2_JACK_SESSION']))
	autowaf.display_msg(conf, "Unit tests",
	                    bool(conf.env['BUILD_TESTS']))
	autowaf.display_msg(conf, "Dynamic manifest support",
	                    bool(conf.env['SLV2_DYN_MANIFEST']))
	autowaf.display_msg(conf, "UI support",
	                    bool(conf.env['SLV2_WITH_UI']))
	autowaf.display_msg(conf, "Python bindings",
	                    bool(conf.env['SLV2_SWIG']))
	print

def build(bld):
	# C/C++ Headers
	bld.install_files('${INCLUDEDIR}/slv2', bld.path.ant_glob('slv2/*.h'))
	bld.install_files('${INCLUDEDIR}/slv2', bld.path.ant_glob('slv2/*.hpp'))

	# Pkgconfig file
	autowaf.build_pc(bld, 'SLV2', SLV2_VERSION, ['SORD','GLIB'])

	lib_source = '''
		src/collections.c
		src/plugin.c
		src/pluginclass.c
		src/plugininstance.c
		src/plugins.c
		src/port.c
		src/query.c
		src/scalepoint.c
		src/util.c
		src/value.c
		src/world.c
	'''.split()

	if bld.env['SLV2_WITH_UI']:
		lib_source += '''
			src/pluginui.c
			src/pluginuiinstance.c
		'''.split()

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
	obj.linkflags       = [ '-ldl' ]
	autowaf.use_lib(bld, obj, 'SORD SERD LV2CORE GLIB')

	if bld.env['BUILD_TESTS']:
		# Static library (for unit test code coverage)
		obj = bld(features = 'c cstlib')
		obj.source       = lib_source
		obj.includes     = ['.', './src']
		obj.name         = 'libslv2_static'
		obj.target       = 'slv2_static'
		obj.install_path = ''
		obj.cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ]
		obj.linkflags    = [ '-ldl' ]
		autowaf.use_lib(bld, obj, 'SORD SERD LV2CORE GLIB')

		# Unit test program
		obj = bld(features = 'c cprogram')
		obj.source       = 'test/slv2_test.c'
		obj.includes     = ['.', './src']
		obj.use          = 'libslv2_static'
		obj.uselib       = 'SORD SERD LV2CORE'
		obj.linkflags    = '-lgcov -ldl'
		obj.target       = 'test/slv2_test'
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

	# JACK Host
	if bld.env['USE_JACK']:
		obj = bld(features = 'c cprogram')
		obj.source       = 'utils/lv2_jack_host.c'
		obj.includes     = ['.', './src', './utils']
		obj.uselib       = 'JACK'
		obj.use          = 'libslv2'
		obj.target       = 'utils/lv2_jack_host'
		obj.install_path = '${BINDIR}'
		if bld.env['SLV2_JACK_SESSION']:
			autowaf.use_lib(bld, obj, 'GLIB GTHREAD')

	# Documentation
	autowaf.build_dox(bld, 'SLV2', SLV2_VERSION, top, out)

	# Bash completion
	if bld.env['BASH_COMPLETION']:
		bld.install_as(
			'/etc/bash_completion.d/slv2', 'utils/slv2.bash_completion')

	if bld.env['SLV2_SWIG']:
		# Python Wrapper
		obj = bld(
			features   = 'cxx cxxshlib pyext',
			source     = 'swig/slv2.i',
			target     = 'swig/_slv2',
			includes   = ['..'],
			swig_flags = '-c++ -python -Wall -I.. -lslv2 -features autodoc=1',
			vnum       = SLV2_LIB_VERSION,
			use        = 'libslv2')
		autowaf.use_lib(bld, obj, 'SLV2')

		bld.install_files('${PYTHONDIR}', 'swig/slv2.py')

	bld.add_post_fun(autowaf.run_ldconfig)

def test(ctx):
	autowaf.pre_test(ctx, APPNAME)
	autowaf.run_tests(ctx, APPNAME, ['test/slv2_test'], dirs=['./src','./test'])
	autowaf.post_test(ctx, APPNAME)
