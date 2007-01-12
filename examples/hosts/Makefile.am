AM_CFLAGS = -std=c99 -I. -I$(top_srcdir)/include -I$(top_srcdir) `pkg-config --cflags rasqal`
AM_LDFLAGS = `pkg-config --libs rasqal`

bin_PROGRAMS = slv2_test_host slv2_simple_jack_host slv2_host

slv2_test_host_DEPENDENCIES = ../../src/libslv2.la
slv2_test_host_LDADD        = ../../src/libslv2.la

slv2_test_host_SOURCES = \
	test_host.c


if WITH_JACK

slv2_simple_jack_host_CFLAGS = @JACK_CFLAGS@ $(AM_CFLAGS)
#simple_jack_host_LIBS = @JACK_LIBS@

slv2_simple_jack_host_DEPENDENCIES = ../../src/libslv2.la
slv2_simple_jack_host_LDADD        = ../../src/libslv2.la @JACK_LIBS@

slv2_simple_jack_host_SOURCES = \
	simple_jack_host.c


slv2_host_CFLAGS = @JACK_CFLAGS@ $(AM_CFLAGS)
#host_LIBS = @JACK_LIBS@

slv2_host_DEPENDENCIES = ../../src/libslv2.la
slv2_host_LDADD        = ../../src/libslv2.la @JACK_LIBS@

slv2_host_SOURCES = \
	host.c


endif # WITH_JACK
