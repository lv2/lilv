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
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.add_option('--no-jack', action='store_true', default=False, dest='no_jack',
			help="Do not build JACK clients")
	opt.add_option('--dyn-manifest', action='store_true', default=False, dest='dyn_manifest',
			help="Build support for dynamic manifest extension [false]")
	opt.add_option('--test', action='store_true', default=False, dest='build_tests',
			help="Build unit tests")
	opt.add_option('--bash-completion', action='store_true', default=False, dest='bash_completion',
			help="Install bash completion script in /etc/bash_completion.d")

def configure(conf):
	autowaf.configure(conf)
	autowaf.display_header('SLV2 Configuration')
	conf.check_tool('compiler_cc')
	autowaf.check_pkg(conf, 'lv2core', uselib_store='LV2CORE', mandatory=True)
	autowaf.check_pkg(conf, 'redland', uselib_store='REDLAND', atleast_version='1.0.6', mandatory=True)
	autowaf.check_pkg(conf, 'jack', uselib_store='JACK', atleast_version='0.107.0', mandatory=False)
	conf.env.append_value('CCFLAGS', '-std=c99')
	conf.define('SLV2_VERSION', SLV2_VERSION)
	if Options.options.dyn_manifest:
		conf.define('SLV2_DYN_MANIFEST', 1)
	conf.write_config_header('slv2-config.h')

	conf.env['USE_JACK'] = conf.env['HAVE_JACK'] and not Options.options.no_jack
	conf.env['BUILD_TESTS'] = Options.options.build_tests
	conf.env['BASH_COMPLETION'] = Options.options.bash_completion

	autowaf.print_summary(conf)
	autowaf.display_msg(conf, "Jack clients", str(conf.env['USE_JACK']))
	autowaf.display_msg(conf, "Unit tests", str(conf.env['BUILD_TESTS']))
	autowaf.display_msg(conf, "Dynamic Manifest Support", str(conf.env['SLV2_DYN_MANIFEST'] == 1))
	print

tests = '''
	test/slv2_test
'''

def build(bld):
	# C Headers
	bld.install_files('${INCLUDEDIR}/slv2', 'slv2/*.h')

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
	obj = bld.new_task_gen('cc', 'shlib')
	obj.export_incdirs = ['.']
	obj.source       = lib_source
	obj.includes     = ['.', './src']
	obj.name         = 'libslv2'
	obj.target       = 'slv2'
	obj.vnum         = SLV2_LIB_VERSION
	obj.install_path = '${LIBDIR}'
	obj.ccflags      = [ '-ldl' ]
	autowaf.use_lib(bld, obj, 'REDLAND LV2CORE')

	if bld.env['BUILD_TESTS']:
		# Static library (for unit test code coverage)
		obj = bld.new_task_gen('cc', 'staticlib')
		obj.source       = lib_source
		obj.includes     = ['.', './src']
		obj.name         = 'libslv2_static'
		obj.target       = 'slv2_static'
		obj.install_path = ''
		obj.ccflags      = [ '-fprofile-arcs',  '-ftest-coverage' ]

		# Unit tests
		for i in tests.split():
			obj = bld.new_task_gen('cc', 'program')
			obj.source       = i + '.c'
			obj.includes     = ['.', './src']
			obj.uselib_local = 'libslv2_static'
			obj.uselib       = 'REDLAND LV2CORE'
			obj.libs         = 'gcov'
			obj.target       = i
			obj.install_path = ''
			obj.ccflags      = [ '-fprofile-arcs',  '-ftest-coverage' ]

	# Utilities
	utils = '''
		utils/lv2_inspect
		utils/lv2_list
	'''
	for i in utils.split():
		obj = bld.new_task_gen('cc', 'program')
		obj.source       = i + '.c'
		obj.includes     = ['.', './src', './utils']
		obj.uselib_local = 'libslv2'
		obj.target       = i
		obj.install_path = '${BINDIR}'

	# JACK Hosts
	hosts = '''
		hosts/lv2_jack_host
	'''
	if bld.env['USE_JACK']:
		for i in hosts.split():
			obj = bld.new_task_gen('cc', 'program')
			obj.source       = i + '.c'
			obj.includes     = ['.', './src', './utils']
			obj.uselib       = 'JACK'
			obj.uselib_local = 'libslv2'
			obj.target       = i
			obj.install_path = '${BINDIR}'

	# Documentation
	autowaf.build_dox(bld, 'SLV2', SLV2_VERSION, srcdir, blddir)
	bld.install_files('${HTMLDIR}', blddir + '/default/doc/html/*')
	bld.install_files('${MANDIR}/man3', blddir + '/default/doc/man/man3/*')
	bld.install_files('${MANDIR}/man1', 'doc/*.1')

	# Bash completion
	if bld.env['BASH_COMPLETION']:
		bld.install_as(
			'/etc/bash_completion.d/slv2', 'utils/slv2.bash_completion')

def test(ctx):
	autowaf.run_tests(ctx, APPNAME, tests.split())

def shutdown():
	autowaf.shutdown()
