AM_CFLAGS = -std=c99 -I. -I$(top_srcdir)/include -I$(top_srcdir) `pkg-config --cflags rasqal`
AM_LDFLAGS = `pkg-config --libs rasqal`

bin_PROGRAMS = test_host jack_host

test_host_DEPENDENCIES = ../../src/libslv2.la
test_host_LDADD        = ../../src/libslv2.la

test_host_SOURCES = \
	test_host.c


if WITH_JACK

jack_host_CFLAGS = @JACK_CFLAGS@ $(AM_CFLAGS)
#jack_host_LIBS = @JACK_LIBS@

jack_host_DEPENDENCIES = ../../src/libslv2.la
jack_host_LDADD        = ../../src/libslv2.la @JACK_LIBS@

jack_host_SOURCES = \
	jack_host.c

endif # WITH_JACK
