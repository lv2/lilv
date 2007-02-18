AM_CFLAGS = -std=c99 -I$(top_srcdir)/include -I$(top_srcdir) @RASQAL_CFLAGS@ -DLIBSLV2_SOURCE -DLV2_TTL_PATH=\"@lv2ttlpath@\"
AM_LDFLAGS = `pkg-config --libs rasqal`

lib_LTLIBRARIES = libslv2.la
libslv2_la_LIBADD = @RASQAL_LIBS@

libslv2_la_SOURCES = \
	private_types.h \
	util.h \
	plugin.c \
	query.c \
	port.c \
	pluginlist.c \
	util.c \
	plugininstance.c  \
	library.c \
	stringlist.c
