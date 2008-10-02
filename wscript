#!/usr/bin/env python
import Params
import autowaf

# Version of this package (even if built as a child)
SLV2_VERSION = '0.6.1'

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
SLV2_LIB_VERSION = '9.0.0'

# Variables for 'waf dist'
APPNAME = 'slv2'
VERSION = SLV2_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.tool_options('compiler_cc')

def configure(conf):
	autowaf.configure(conf)
	if not conf.env['CC']:
		conf.check_tool('compiler_cc')
	if not conf.env['HAVE_LV2CORE']:
		conf.check_pkg('lv2core', destvar='LV2CORE', vnum='1', mandatory=True)
	if not conf.env['HAVE_REDLAND']:
		conf.check_pkg('redland', destvar='REDLAND', vnum='1.0.8', mandatory=True)
	if not conf.env['HAVE_JACK']:
		conf.check_pkg('jack', destvar='JACK', vnum='0.107.0', mandatory=False)
	conf.env.append_value('CCFLAGS', '-std=c99')
	conf.define('PACKAGE_VERSION', VERSION)
	conf.write_config_header('waf-config.h')
	conf.env.append_value('CCFLAGS', '-DCONFIG_H_PATH=\\\"waf-config.h\\\"')
	conf.env.append_value('CCFLAGS', '-DPACKAGE_VERSION=\\\"' + VERSION + '\\\"')
	
	autowaf.print_summary(conf)
	autowaf.display_header('SLV2 Configuration')
	autowaf.display_msg("Jack clients", str(bool(conf.env['HAVE_JACK'])), 'YELLOW')
	print
		
def build(bld):
	# C Headers
	install_files('PREFIX', 'include/slv2', 'slv2/*.h')

	# Pkgconfig file
	autowaf.build_pc(bld, 'SLV2', SLV2_VERSION, ['REDLAND'])
	
	# Library
	obj = bld.create_obj('cc', 'shlib')
	obj.source = '''
		src/plugin.c
		src/pluginclass.c
		src/pluginclasses.c
		src/plugininstance.c
		src/plugins.c
		src/pluginui.c
		src/pluginuiinstance.c
		src/pluginuis.c
		src/port.c
		src/query.c
		src/scalepoint.c
		src/scalepoints.c
		src/util.c
		src/value.c
		src/values.c
		src/world.c
	'''
	obj.includes = '..'
	obj.name     = 'libslv2'
	obj.target   = 'slv2'
	obj.vnum     = SLV2_LIB_VERSION
	autowaf.use_lib(bld, obj, 'REDLAND LV2CORE')

	# Utilities
	utils = '''
		utils/lv2_inspect
		utils/lv2_list
	'''
	for i in utils.split():
		obj = bld.create_obj('cc', 'program')
		obj.source       = i + '.c'
		obj.includes     = '.'
		obj.uselib_local = 'libslv2'
		obj.target       = i
	
	# JACK Hosts
	hosts = '''
		hosts/lv2_jack_host
		hosts/lv2_simple_jack_host
	'''
	for i in hosts.split():
		obj = bld.create_obj('cc', 'program')
		obj.source       = i + '.c'
		obj.includes     = '.'
		obj.uselib       = 'JACK'
		obj.uselib_local = 'libslv2'
		obj.target       = i

