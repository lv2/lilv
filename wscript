#!/usr/bin/env python
import os
import sys
import subprocess

from waflib.extras import autowaf as autowaf
import waflib.Options as Options
import waflib.Logs as Logs

# Version of this package (even if built as a child)
LILV_VERSION       = '0.9.0'
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
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('python')
    autowaf.set_options(opt)
    opt.add_option('--no-utils', action='store_true', default=False, dest='no_utils',
                   help="Do not build command line utilities")
    opt.add_option('--bindings', action='store_true', default=False, dest='bindings',
                   help="Build python bindings")
    opt.add_option('--dyn-manifest', action='store_true', default=False,
                   dest='dyn_manifest',
                   help="Build support for dynamic manifests")
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--no-bash-completion', action='store_true', default=False,
                   dest='no_bash_completion',
                   help="Don't install bash completion script in CONFIGDIR")
    opt.add_option('--default-lv2-path', type='string', default='',
                   dest='default_lv2_path',
                   help="Default LV2 path to use if $LV2_PATH is unset (globs and ~ supported)")
    opt.add_option('--default-state-bundle', type='string', default='',
                   dest='default_state_bundle',
                   help="Default path to use for user-writable state/preset bundle")
    opt.add_option('--static', action='store_true', default=False, dest='static',
                   help="Build static library")

def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.load('python')

    if Options.options.bindings:
        try:
            conf.load('swig python')
            conf.check_python_headers()
            autowaf.define(conf, 'LILV_PYTHON', 1);
        except:
            pass

    conf.line_just = 51
    autowaf.configure(conf)
    autowaf.display_header('Lilv Configuration')
    conf.env.append_unique('CFLAGS', '-std=c99')

    autowaf.check_pkg(conf, 'lv2core', uselib_store='LV2CORE', mandatory=True)
    autowaf.check_pkg(conf, 'serd-0', uselib_store='SERD',
                      atleast_version='0.9.0', mandatory=True)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.5.0', mandatory=True)
    autowaf.check_pkg(conf, 'lv2-lv2plug.in-ns-ext-urid',
                      uselib_store='LV2_URID', mandatory=True)
    autowaf.check_pkg(conf, 'lv2-lv2plug.in-ns-ext-state',
                      uselib_store='LV2_STATE', mandatory=False)

    defines = ['_POSIX_C_SOURCE', '_BSD_SOURCE']
    if Options.platform == 'darwin':
        defines += ['_DARWIN_C_SOURCE']

    conf.check_cc(function_name='wordexp',
                  header_name='wordexp.h',
                  defines=defines,
                  define_name='HAVE_WORDEXP',
                  mandatory=False)

    conf.check_cc(function_name='flock',
                  header_name='sys/file.h',
                  defines=defines,
                  define_name='HAVE_FLOCK',
                  mandatory=False)

    conf.check_cc(function_name='fileno',
                  header_name='stdio.h',
                  defines=defines,
                  define_name='HAVE_FILENO',
                  mandatory=False)

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

    # Set default LV2 path
    lv2_path = Options.options.default_lv2_path
    if lv2_path == '':
        if Options.platform == 'darwin':
            lv2_path = lilv_path_sep.join(['~/Library/Audio/Plug-Ins/LV2',
                                           '~/.lv2',
                                           '/usr/local/lib/lv2',
                                           '/usr/lib/lv2',
                                           '/Library/Audio/Plug-Ins/LV2'])
        elif Options.platform == 'haiku':
            lv2_path = lilv_path_sep.join(['~/.lv2',
                                           '/boot/common/add-ons/lv2'])
        elif Options.platform == 'win32':
            lv2_path = lilv_path_sep.join(['%APPDATA%\\\\LV2',
                                           '%PROGRAMFILES%\\\\LV2'])
        else:
            libdirname = os.path.basename(conf.env['LIBDIR'])
            lv2_path = lilv_path_sep.join(['~/.lv2',
                                           '/usr/%s/lv2' % libdirname,
                                           '/usr/local/%s/lv2' % libdirname])
    autowaf.define(conf, 'LILV_DEFAULT_LV2_PATH', lv2_path)

    # Set default state bundle
    state_bundle = Options.options.default_state_bundle
    if state_bundle == '':
        if Options.platform == 'darwin':
            state_bundle = '~/Library/Audio/Plug-Ins/LV2/presets.lv2'
        elif Options.platform == 'win32':
            state_bundle = '%APPDATA%\\\\LV2\\\\presets.lv2'
        else:
            state_bundle = '~/.lv2/presets.lv2'
    autowaf.define(conf, 'LILV_DEFAULT_STATE_BUNDLE', state_bundle)

    conf.env['BUILD_TESTS']     = Options.options.build_tests
    conf.env['BUILD_UTILS']     = not Options.options.no_utils
    conf.env['BUILD_STATIC']    = Options.options.static
    conf.env['BASH_COMPLETION'] = not Options.options.no_bash_completion

    conf.env['LIB_LILV'] = ['lilv-%s' % LILV_MAJOR_VERSION]

    conf.write_config_header('lilv-config.h', remove=False)

    autowaf.display_msg(conf, "Default LV2_PATH",
                        conf.env['LILV_DEFAULT_LV2_PATH'])
    autowaf.display_msg(conf, "Default state/preset bundle",
                        conf.env['LILV_DEFAULT_STATE_BUNDLE'])
    autowaf.display_msg(conf, "Utilities",
                        bool(conf.env['BUILD_UTILS']))
    autowaf.display_msg(conf, "Unit tests",
                        bool(conf.env['BUILD_TESTS']))
    autowaf.display_msg(conf, "Dynamic manifest support",
                        bool(conf.env['LILV_DYN_MANIFEST']))
    autowaf.display_msg(conf, "State/Preset support",
                        conf.is_defined('HAVE_LV2_STATE'))
    autowaf.display_msg(conf, "Python bindings",
                        conf.is_defined('LILV_PYTHON'))
    print('')

def build(bld):
    # C/C++ Headers
    includedir = '${INCLUDEDIR}/lilv-%s/lilv' % LILV_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('lilv/*.h'))
    bld.install_files(includedir, bld.path.ant_glob('lilv/*.hpp'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'LILV', LILV_VERSION, LILV_MAJOR_VERSION, [],
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
        src/state.c
        src/ui.c
        src/util.c
        src/world.c
        src/zix/tree.c
    '''.split()

    lib = [ 'dl' ]
    libflags  = [ '-fvisibility=hidden' ]
    if sys.platform == 'win32':
        lib = []
        libflags  = []
    elif sys.platform.find('bsd') > 0:
        lib = []

    # Shared Library
    obj = bld(features        = 'c cshlib',
              export_includes = ['.'],
              source          = lib_source,
              includes        = ['.', './src'],
              name            = 'liblilv',
              target          = 'lilv-%s' % LILV_MAJOR_VERSION,
              vnum            = LILV_LIB_VERSION,
              install_path    = '${LIBDIR}',
              cflags          = libflags + [ '-DLILV_SHARED',
                                             '-DLILV_INTERNAL' ],
              lib             = lib)
    autowaf.use_lib(bld, obj, 'SORD LV2CORE LV2_STATE LV2_URID')

    # Static library
    if bld.env['BUILD_STATIC']:
        obj = bld(features        = 'c cstlib',
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  name            = 'liblilv_static',
                  target          = 'lilv-%s' % LILV_MAJOR_VERSION,
                  vnum            = LILV_LIB_VERSION,
                  install_path    = '${LIBDIR}',
                  cflags          = [ '-DLILV_INTERNAL' ])
        autowaf.use_lib(bld, obj, 'SORD LV2CORE LV2_STATE LV2_URID')

    if bld.env['BUILD_TESTS']:
        # Test plugin library
        penv          = bld.env.derive()
        shlib_pattern = penv['cshlib_PATTERN']
        if shlib_pattern.startswith('lib'):
            shlib_pattern = shlib_pattern[3:]
        penv['cshlib_PATTERN'] = shlib_pattern
        shlib_ext = shlib_pattern[shlib_pattern.rfind('.'):]

        obj = bld(features     = 'c cshlib',
                  env          = penv,
                  source       = 'test/test_plugin.c',
                  name         = 'test_plugin',
                  target       = 'test/test_plugin.lv2/test_plugin',
                  install_path = None,
                  uselib       = 'LV2CORE LV2_STATE LV2_URID')

        # Test plugin data files
        for i in [ 'manifest.ttl.in', 'test_plugin.ttl.in' ]:
            bld(features     = 'subst',
                source       = 'test/' + i,
                target       = 'test/test_plugin.lv2/' + i.replace('.in', ''),
                install_path = None,
                SHLIB_EXT    = shlib_ext)

        # Static profiled library (for unit test code coverage)
        obj = bld(features     = 'c cstlib',
                  source       = lib_source,
                  includes     = ['.', './src'],
                  name         = 'liblilv_profiled',
                  target       = 'lilv_profiled',
                  install_path = '',
                  cflags       = [ '-fprofile-arcs', '-ftest-coverage',
                                   '-DLILV_INTERNAL' ],
                  lib          = lib + ['gcov'])
        autowaf.use_lib(bld, obj, 'SORD LV2CORE LV2_STATE LV2_URID')

        # Unit test program
        obj = bld(features     = 'c cprogram',
                  source       = 'test/lilv_test.c',
                  includes     = ['.', './src'],
                  use          = 'liblilv_profiled',
                  uselib       = 'SORD LV2CORE',
                  lib          = lib + ['gcov'],
                  target       = 'test/lilv_test',
                  install_path = '',
                  defines      = ['LILV_TEST_BUNDLE=\"%s/\"' % os.path.abspath(
                    os.path.join(out, 'test', 'test_plugin.lv2'))],
                  cflags       = [ '-fprofile-arcs',  '-ftest-coverage' ])
        autowaf.use_lib(bld, obj, 'SORD LV2CORE LV2_STATE LV2_URID')

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

    # Documentation
    autowaf.build_dox(bld, 'LILV', LILV_VERSION, top, out)

    # Man pages
    bld.install_files('${MANDIR}/man1', bld.path.ant_glob('doc/*.1'))

    # Bash completion
    if bld.env['BASH_COMPLETION']:
        bld.install_as(
            '${SYSCONFDIR}/bash_completion.d/lilv', 'utils/lilv.bash_completion')

    if bld.is_defined('LILV_PYTHON'):
        # Python Wrapper
        obj = bld(features   = 'cxx cxxshlib pyext',
                  source     = 'bindings/lilv.i',
                  target     = 'bindings/_lilv',
                  includes   = ['..'],
                  swig_flags = '-c++ -python -Wall -I.. -llilv -features autodoc=1',
                  use        = 'liblilv')
        autowaf.use_lib(bld, obj, 'LILV')

        bld.install_files('${PYTHONDIR}', 'bindings/lilv.py')

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env['DOCS']:
        bld.add_post_fun(fix_docs)

def build_dir(ctx, subdir):
    if autowaf.is_child():
        return os.path.join('build', APPNAME, subdir)
    else:
        return os.path.join('build', subdir)

def fix_docs(ctx):
    try:
        top = os.getcwd()
        os.chdir(build_dir(ctx, 'doc/html'))
        os.system("sed -i 's/LILV_API //' group__lilv.html")
        os.system("sed -i 's/LILV_DEPRECATED //' group__lilv.html")
        os.remove('index.html')
        os.symlink('group__lilv.html',
                   'index.html')
        os.chdir(top)
        os.chdir(build_dir(ctx, 'doc/man/man3'))
        os.system("sed -i 's/LILV_API //' lilv.3")
        os.chdir(top)
    except:
        Logs.error("Failed to fix up %s documentation" % APPNAME)

def upload_docs(ctx):
    os.system("rsync -ravz --delete -e ssh build/doc/html/ drobilla@drobilla.net:~/drobilla.net/docs/lilv/")

def test(ctx):
    autowaf.pre_test(ctx, APPNAME)
    autowaf.run_tests(ctx, APPNAME, ['test/lilv_test'], dirs=['./src','./test'])
    autowaf.post_test(ctx, APPNAME)

def lint(ctx):
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include src/* lilv/*', shell=True)
