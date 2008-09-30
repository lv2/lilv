#!/usr/bin/env python
import Params

# Variables for 'waf dist'
VERSION = '0.6.1'
APPNAME = 'slv2'

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('compiler_cc')

def configure(conf):
	if not conf.env['HAVE_LV2CORE']:
		conf.check_pkg('lv2core', destvar='LV2CORE', vnum='1', mandatory=True)
	if not conf.env['HAVE_REDLAND']:
		conf.check_pkg('redland', destvar='REDLAND', vnum='1.0.8', mandatory=True)
	if not conf.env['HAVE_JACK']:
		conf.check_pkg('jack', destvar='JACK', vnum='0.107.0', mandatory=False)
	if not conf.env['CC']:
		conf.check_tool('compiler_cc')
	conf.env.append_value('CCFLAGS', '-std=c99')
	conf.define('PACKAGE_VERSION', VERSION)
	conf.write_config_header('waf-config.h')
	conf.env.append_value('CCFLAGS', '-DCONFIG_H_PATH=\\\"waf-config.h\\\"')
	conf.env.append_value('CCFLAGS', '-DPACKAGE_VERSION=\\\"' + VERSION + '\\\"')

def build(bld):
	# Headers (slv2)
	install_files('PREFIX', 'include/slv2', 'slv2/*.h')
	
	# Library (src)
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
	obj.uselib   = 'REDLAND LV2CORE'
	obj.vnum     = '1.0.0'

	# Utilities (util)
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
		obj.inst_var     = 0
	
	# JACK Hosts (hosts)
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
		obj.inst_var     = 0

