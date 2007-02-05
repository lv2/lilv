AC_PREREQ(2.59)
AC_INIT([libslv2],[0.0.1],[dave@drobilla.net])
AC_CONFIG_SRCDIR([src/plugin.c])
AC_CONFIG_SRCDIR([slv2/plugin.h])
AC_CONFIG_SRCDIR([utils/lv2_list.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for compiler
AC_PROG_CC

# Check pedantic other stuff autoscan says we should :)
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Library building stuff
AC_PROG_LIBTOOL

# Check for debugging flag
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (false)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  CFLAGS="-O0 -g -DDEBUG"
  CXXFLAGS="-O0 -g -DDEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CFLAGS -DNDEBUG"
fi

# Check for strict flag
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (false)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  CFLAGS="$CFLAGS -std=c99 -pedantic -Wall -Wextra -Wconversion -Winit-self"
  CXXFLAGS="$CFLAGS -ansi -pedantic -Wall -Wextra -Wconversion -Winit-self"
fi


# Check for RAPTOR
PKG_CHECK_MODULES(RAPTOR, raptor >= 0.21, build_raptor="yes", build_raptor="no")

# Check for RASQAL
PKG_CHECK_MODULES(RASQAL, rasqal >= 0.9.11, build_rasqal="yes", build_rasqal="no")

# Check for JACK
build_jack="yes"
AC_ARG_ENABLE(jack,
	[AS_HELP_STRING(--enable-jack, [Build JACK clients (true)])],
	[ if test x$enable_jack = xno ; then build_jack=no ; fi ])
if test "$build_jack" = "yes"; then
	PKG_CHECK_MODULES(JACK, jack >= 0.102.29, build_jack="yes", build_jack="no")
	AC_DEFINE(HAVE_JACK, 1, [Has JACK])
else
	AC_MSG_WARN("Sufficiently recent JACK not found, JACK clients will not be built.")
fi
AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])

# Check plugin install directory
#AC_MSG_CHECKING([where to install LV2 plugins])
#AC_ARG_WITH(lv2-dir,
#	AS_HELP_STRING([--with-lv2-dir=DIR],
#		[directory that LV2 plugins should be installed in ($libdir/lv2)]),
#	[lv2dir=$withval], [lv2dir=$libdir/lv2])
#AC_MSG_RESULT($lv2dir)
#AC_SUBST(lv2dir)

# Build ladspa converter?
build_ladspa2lv2="no"
AC_ARG_ENABLE(ladspa2lv2,
	[AS_HELP_STRING(--enable-ladspa2lv2, [Build ladspa2lv2 (no) - BROKEN])],
	[build_ladspa2lv2="$enableval"])
if test "$build_ladspa2lv2" = "yes"; then
  AC_PROG_CXX
  PKG_CHECK_MODULES(RAUL, raul >= 0.0.0)
fi
AM_CONDITIONAL(BUILD_LADSPA2SLV2, [test "$build_ladspa2lv2" = "yes"])

# Write output files
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([slv2/Makefile])
AC_CONFIG_FILES([utils/Makefile])
AC_CONFIG_FILES([hosts/Makefile])
AC_CONFIG_FILES([data/Makefile])
AC_CONFIG_FILES([libslv2.pc])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/reference.doxygen])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([SLV2 build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building JACK clients: $build_jack])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

