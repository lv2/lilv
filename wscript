#!/usr/bin/env python
import os
import sys
import subprocess

from waflib.extras import autowaf as autowaf
import waflib.Options as Options
import waflib.Logs as Logs

# Version of this package (even if built as a child)
LILV_VERSION       = '0.4.0'
LILV_MAJOR_VERSION = '0'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Lilv uses the same version number for both library and package
LILV_LIB_VERSION = LILV_VERSION

# Variables for 'waf dist'
APPNAME = 'lilv'
VERSION = LILV_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    autowaf.set_options(opt)
    opt.load('python')
    opt.add_option('--no-utils', action='store_true', default=False, dest='no_utils',
                   help="Do not build command line utilities")
    opt.add_option('--no-jack', action='store_true', default=False, dest='no_jack',
                   help="Do not build JACK clients")
    opt.add_option('--no-jack-session', action='store_true', default=False,
                   dest='no_jack_session',
                   help="Do not build JACK session support")
    opt.add_option('--bindings', action='store_true', default=False, dest='bindings',
                   help="Build python bindings")
    opt.add_option('--dyn-manifest', action='store_true', default=False,
                   dest='dyn_manifest',
                   help="Build support for dynamic manifests")
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--no-bash-completion', action='store_true', default=False,
                   dest='no_bash_completion',
                   help="Install bash completion script in /etc/bash_completion.d")
    opt.add_option('--default-lv2-path', type='string', default='',
                   dest='default_lv2_path',
                   help="Default LV2 path to use if $LV2_PATH is unset (globs and ~ supported)")

def configure(conf):
    conf.line_just = 63
    autowaf.configure(conf)
    autowaf.display_header('Lilv Configuration')
    conf.load('compiler_cc')
    conf.load('python')

    if Options.options.bindings:
        try:
            conf.load('swig python')
            conf.check_python_headers()
            autowaf.define(conf, 'LILV_BINDINGS', 1);
        except:
            pass

    autowaf.check_pkg(conf, 'lv2core', uselib_store='LV2CORE', mandatory=True)
    autowaf.check_pkg(conf, 'glib-2.0', uselib_store='GLIB',
                      atleast_version='2.0.0', mandatory=True)
    autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
                      atleast_version='2.0.0', mandatory=False)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.3.0', mandatory=True)
    autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
                      atleast_version='0.107.0', mandatory=False)
    autowaf.check_pkg(conf, 'jack', uselib_store='NEW_JACK',
                      atleast_version='0.120.0', mandatory=False)

    conf.check(function_name='wordexp',
               header_name='wordexp.h',
               define_name='HAVE_WORDEXP',
               mandatory=False)

    autowaf.check_header(conf, 'lv2/lv2plug.in/ns/lv2core/lv2.h')

    if not Options.options.no_jack_session:
        if conf.is_defined('HAVE_NEW_JACK') and conf.is_defined('HAVE_GTHREAD'):
            autowaf.define(conf, 'LILV_JACK_SESSION', 1)

    conf.env.append_value('CFLAGS', '-std=c99')
    autowaf.define(conf, 'LILV_VERSION', LILV_VERSION)
    if Options.options.dyn_manifest:
        autowaf.define(conf, 'LILV_DYN_MANIFEST', 1)

    lilv_path_sep = ':'
    lilv_dir_sep  = '/'
    if sys.platform == 'win32':
        lilv_path_sep = ';'
        lilv_dir_sep = '\\\\'

    autowaf.define(conf, 'LILV_PATH_SEP', lilv_path_sep)
    autowaf.define(conf, 'LILV_DIR_SEP',  lilv_dir_sep)

    if Options.options.default_lv2_path == '':
        if Options.platform == 'darwin':
            Options.options.default_lv2_path = lilv_path_sep.join([
                    '~/Library/Audio/Plug-Ins/LV2',
                    '~/.lv2',
                    '/usr/local/lib/lv2',
                    '/usr/lib/lv2',
                    '/Library/Audio/Plug-Ins/LV2'])
        elif Options.platform == 'haiku':
            Options.options.default_lv2_path = lilv_path_sep.join([
                    '~/.lv2',
                    '/boot/common/add-ons/lv2'])
        elif Options.platform == 'win32':
            Options.options.default_lv2_path = '%APPDATA%\\\\LV2;%PROGRAMFILES%\\\\LV2'
        else:
            libdirname = os.path.basename(conf.env['LIBDIR'])
            Options.options.default_lv2_path = lilv_path_sep.join([
                    '~/.lv2',
                    '/usr/%s/lv2' % libdirname,
                    '/usr/local/%s/lv2' % libdirname])

    autowaf.define(conf, 'LILV_DEFAULT_LV2_PATH', Options.options.default_lv2_path)

    conf.env['BUILD_TESTS']     = Options.options.build_tests
    conf.env['BUILD_UTILS']     = not Options.options.no_utils
    conf.env['BASH_COMPLETION'] = not Options.options.no_bash_completion

    if conf.is_defined('HAVE_JACK') and not Options.options.no_jack:
        autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event.h',
                             'HAVE_LV2_EVENT')
        autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event-helpers.h',
                             'HAVE_LV2_EVENT_HELPERS')
        autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-map/uri-map.h',
                             'HAVE_LV2_URI_MAP')
        if (conf.is_defined('HAVE_LV2_EVENT')
            and conf.is_defined('HAVE_LV2_EVENT_HELPERS')
            and conf.is_defined('HAVE_LV2_URI_MAP')):
            autowaf.define(conf, 'LILV_USE_JACK', 1)

    conf.write_config_header('lilv-config.h', remove=False)

    autowaf.display_msg(conf, "Default LV2_PATH",
                        conf.env['LILV_DEFAULT_LV2_PATH'])
    autowaf.display_msg(conf, "Utilities",
                        bool(conf.env['BUILD_UTILS']))
    autowaf.display_msg(conf, "Jack clients",
                        bool(conf.is_defined('LILV_USE_JACK')))
    autowaf.display_msg(conf, "Jack session support",
                        bool(conf.env['LILV_JACK_SESSION']))
    autowaf.display_msg(conf, "Unit tests",
                        bool(conf.env['BUILD_TESTS']))
    autowaf.display_msg(conf, "Dynamic manifest support",
                        bool(conf.env['LILV_DYN_MANIFEST']))
    autowaf.display_msg(conf, "Python bindings",
                        conf.is_defined('LILV_PYTHON'))
    print('')

def build(bld):
    # C/C++ Headers
    includedir = '${INCLUDEDIR}/lilv-%s/lilv' % LILV_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('lilv/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('lilv/*.hpp'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'LILV', LILV_VERSION, LILV_MAJOR_VERSION, ['SORD','GLIB'],
                     {'LILV_MAJOR_VERSION' : LILV_MAJOR_VERSION})

    lib_source = '''
        src/collections.c
        src/instance.c
        src/node.c
        src/plugin.c
        src/pluginclass.c
        src/port.c
        src/query.c
        src/scalepoint.c
        src/ui.c
        src/util.c
        src/world.c
    '''.split()

    linkflags = [ '-ldl' ]
    libflags  = [ '-fvisibility=hidden' ]
    if sys.platform == 'win32':
        linkflags = []
        libflags  = []

    # Library
    obj = bld(features        = 'c cshlib',
              export_includes = ['.'],
              source          = lib_source,
              includes        = ['.', './src'],
              name            = 'liblilv',
              target          = 'lilv-%s' % LILV_MAJOR_VERSION,
              vnum            = LILV_LIB_VERSION,
              install_path    = '${LIBDIR}',
              cflags          = libflags + [
                                  '-DLILV_SHARED',
                                  '-DLILV_INTERNAL' ],
              linkflags       = linkflags)
    autowaf.use_lib(bld, obj, 'SORD SERD LV2CORE GLIB')

    if bld.env['BUILD_TESTS']:
        # Static library (for unit test code coverage)
        obj = bld(features     = 'c cstlib',
                  source       = lib_source,
                  includes     = ['.', './src'],
                  name         = 'liblilv_static',
                  target       = 'lilv_static',
                  install_path = '',
                  cflags       = [ '-fprofile-arcs',  '-ftest-coverage', '-DLILV_INTERNAL' ],
                  linkflags    = linkflags)
        autowaf.use_lib(bld, obj, 'SORD SERD LV2CORE GLIB')

        # Unit test program
        obj = bld(features     = 'c cprogram',
                  source       = 'test/lilv_test.c',
                  includes     = ['.', './src'],
                  use          = 'liblilv_static',
                  uselib       = 'SORD SERD LV2CORE',
                  linkflags    = linkflags + ['-lgcov'],
                  target       = 'test/lilv_test',
                  install_path = '',
                  cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ])

    # Utilities
    if bld.env['BUILD_UTILS']:
        utils = '''
            utils/lv2info
            utils/lv2ls
            utils/lilv-bench
        '''
        for i in utils.split():
            obj = bld(features     = 'c cprogram',
                      source       = i + '.c',
                      includes     = ['.', './src', './utils'],
                      use          = 'liblilv',
                      target       = i,
                      install_path = '${BINDIR}')

    # JACK Host
    if bld.is_defined('LILV_USE_JACK'):
        obj = bld(features     = 'c cprogram',
                  source       = 'utils/lv2jack.c',
                  includes     = ['.', './src', './utils'],
                  uselib       = 'JACK',
                  use          = 'liblilv',
                  target       = 'utils/lv2jack',
                  install_path = '${BINDIR}')
        if bld.is_defined('LILV_JACK_SESSION'):
            autowaf.use_lib(bld, obj, 'GLIB GTHREAD')

    # Documentation
    autowaf.build_dox(bld, 'LILV', LILV_VERSION, top, out)

    # Man pages
    bld.install_files('${MANDIR}/man1', bld.path.ant_glob('doc/*.1'))

    # Bash completion
    if bld.env['BASH_COMPLETION']:
        bld.install_as(
            '/etc/bash_completion.d/lilv', 'utils/lilv.bash_completion')

    if bld.is_defined('LILV_PYTHON'):
        # Python Wrapper
        obj = bld(features   = 'cxx cxxshlib pyext',
                  source     = 'bindings/lilv.i',
                  target     = 'bindings/_lilv',
                  includes   = ['..'],
                  swig_flags = '-c++ -python -Wall -I.. -llilv -features autodoc=1',
                  vnum       = LILV_LIB_VERSION,
                  use        = 'liblilv')
        autowaf.use_lib(bld, obj, 'LILV')

        bld.install_files('${PYTHONDIR}', 'bindings/lilv.py')

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env['DOCS']:
        bld.add_post_fun(fix_docs)

def fix_docs(ctx):
    try:
        top = os.getcwd()
        os.chdir('build/doc/html')
        os.system("sed -i 's/LILV_API //' group__lilv.html")
        os.remove('index.html')
        os.symlink('group__lilv.html',
                   'index.html')
        os.chdir(top)
        os.chdir('build/doc/man/man3')
        os.system("sed -i 's/LILV_API //' lilv.3")
    except Exception, e:
        Logs.error("Failed to fix up Doxygen documentation (%s)\n" % e)

def upload_docs(ctx):
    os.system("rsync -avz --delete -e ssh build/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/lilv")

def test(ctx):
    autowaf.pre_test(ctx, APPNAME)
    autowaf.run_tests(ctx, APPNAME, ['test/lilv_test'], dirs=['./src','./test'])
    autowaf.post_test(ctx, APPNAME)

def lint(ctx):
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include src/* lilv/*', shell=True)
