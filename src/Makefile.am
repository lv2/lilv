AM_CFLAGS = -std=c99 -I$(top_srcdir)/include -I$(top_srcdir) @REDLAND_CFLAGS@ -DLIBSLV2_SOURCE -DLV2_TTL_PATH=\"@lv2ttlpath@\"
AM_LDFLAGS = `pkg-config --libs rasqal`

lib_LTLIBRARIES = libslv2.la
libslv2_la_LIBADD = @REDLAND_LIBS@

libslv2_la_SOURCES = \
	private_types.h \
	world.c \
	pluginclass.c \
	pluginclasses.c \
	plugin.c \
	query.c \
	port.c \
	plugins.c \
	plugininstance.c \
	strings.c \
	util.c
