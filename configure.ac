AC_PREREQ(2.59)

AC_INIT([slv2],0.3.1,[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])

AC_CONFIG_SRCDIR([src/plugin.c])
AC_CONFIG_SRCDIR([slv2/plugin.h])
AC_CONFIG_SRCDIR([utils/lv2_list.c])
AC_CONFIG_SRCDIR([swig/slv2.i])
AC_CONFIG_HEADER([config/config.h])
AM_INIT_AUTOMAKE

# SLV2 interface version (libtool shared library versioning)
#
# current = incremented whenever the public API is changed 
# revision = incremented when the implementation of current is changed
# age = current library is both source and binary compatible with
#       interfaces: current,current-1,...,current-age
#
# See libtool documentation for detailed documentation
#
# Version history:
# 0.0.1 = 0,0,0
# 0.1.0 = 2,0,0
# 0.2.0 = 3,0,0
# 0.3.0 = 4,0,0
# 0.3.1 = 4,0,0

SLV2_VERSION="0.3.1"
SLV2_API_CURRENT="4"
SLV2_API_REVISION="0"
SLV2_API_AGE="0"

AC_SUBST(SLV2_API_CURRENT)
AC_SUBST(SLV2_API_REVISION)
AC_SUBST(SLV2_API_AGE)
SLV2_SO_VERSION="${SLV2_API_CURRENT}:${SLV2_API_REVISION}:${SLV2_API_AGE}"
AC_SUBST(SLV2_SO_VERSION)
AC_SUBST(SLV2_VERSION)

# This is cute... autohell FTW
builddir=`pwd`
cd $srcdir
abs_srcdir=`pwd`
cd $builddir

SLV2_CFLAGS="-I$abs_srcdir"
SLV2_LIBS="$abs_srcdir/src/.libs/libslv2.la"
AC_SUBST(SLV2_CFLAGS)
AC_SUBST(SLV2_LIBS)

CONFIG_H_PATH="$builddir/config/config.h"

# Checks for compiler
AM_PROG_CC_C_O

# Library building stuff
AC_PROG_LIBTOOL

# Check for debugging flag
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (false)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  CFLAGS="-O0 -g -DDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="-O0 -g -DDEBUG -DLIBRDF_DEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CFLAGS -DNDEBUG -DLIBRDF_DEBUG"
fi

# Check for strict flag
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (false)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  CFLAGS="$CFLAGS -std=c99 -pedantic -Wall -Wextra -Winit-self"
  CXXFLAGS="$CFLAGS -ansi -pedantic -Wall -Wextra -Wconversion -Winit-self"
fi

# SWIG bindings
bindings="no"
AC_ARG_ENABLE(bindings,
	[AS_HELP_STRING(--enable-bindings, [Build language bindings via SWIG (false)])],
	[bindings="$enableval"])
if test "$bindings" = "yes"; then
  AC_CHECK_PROGS(SWIG, swig)
fi
AM_CONDITIONAL(WITH_SWIG, [test "$bindings" = "yes"])

# Check for Redland
build_redland="no"
AC_CHECK_PROG(REDLAND_CONFIG, redland-config, redland-config)
if test "X$REDLAND_CONFIG" = X; then
  AC_MSG_ERROR([REDLANDMM requires Redland (librdf), but redland-config not found.])
else
  build_redland="yes"
  REDLAND_CFLAGS=`$REDLAND_CONFIG --cflags`
  REDLAND_LIBS=`$REDLAND_CONFIG --libs`
  AC_SUBST(REDLAND_CFLAGS)
  AC_SUBST(REDLAND_LIBS)
fi
AM_CONDITIONAL(WITH_REDLAND, [test "$build_redland" = "yes"])

# Check for JACK
build_jack="yes"
AC_ARG_ENABLE(jack,
	[AS_HELP_STRING(--enable-jack, [Build JACK clients (true)])],
	[ if test x$enable_jack = xno ; then build_jack=no ; fi ])
if test "$build_jack" = "yes"; then
	PKG_CHECK_MODULES(JACK, jack >= 0.102.29, build_jack="yes", build_jack="no")
	AC_DEFINE(HAVE_JACK, 1, [Has JACK])
	AC_JACK_MIDI_NFRAMES_CHECK()  	   	
	if test "$jackmidi_nframes" == "yes"; then 	  	
		AC_DEFINE([JACK_MIDI_NEEDS_NFRAMES], 1, [Defined if we JACK MIDI functions need nframes parameter.]) 	  	
	fi
else
	AC_MSG_WARN("Sufficiently recent JACK not found, JACK clients will not be built.")
fi
AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])

# Doxygen is required to make dist
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
if test $HAVE_DOXYGEN = "false"; then
	AC_MSG_WARN([Doxygen not found, documentation will not be built])
fi
AM_CONDITIONAL(HAVE_DOXYGEN, $HAVE_DOXYGEN)

# Bolt on a few specific flags to CFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -ansi -pipe -fmessage-length=999 -DCONFIG_H_PATH=\\\"$CONFIG_H_PATH\\\""
CFLAGS="$CFLAGS -std=c99 -pipe -fmessage-length=999 -DCONFIG_H_PATH=\\\"$CONFIG_H_PATH\\\""

# Write output files
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([slv2/Makefile])
AC_CONFIG_FILES([utils/Makefile])
AC_CONFIG_FILES([hosts/Makefile])
AC_CONFIG_FILES([swig/Makefile])
AC_CONFIG_FILES([data/Makefile])
AC_CONFIG_FILES([data/lv2core.lv2/Makefile])
AC_CONFIG_FILES([slv2.pc])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/reference.doxygen])
AC_CONFIG_FILES([debian/Makefile])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([SLV2 build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building JACK clients:     $build_jack])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

