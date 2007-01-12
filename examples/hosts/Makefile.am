AM_CFLAGS = -std=c99 -I$(top_srcdir) @RASQAL_CFLAGS@

bin_PROGRAMS = slv2_test_host

if WITH_JACK
bin_PROGRAMS += slv2_simple_jack_host slv2_host
endif

slv2_test_host_DEPENDENCIES = ../../src/libslv2.la
slv2_test_host_LDADD        = ../../src/libslv2.la

slv2_test_host_SOURCES = \
	test_host.c


if WITH_JACK

slv2_simple_jack_host_CFLAGS = @JACK_CFLAGS@ $(AM_CFLAGS)

slv2_simple_jack_host_DEPENDENCIES = ../../src/libslv2.la
slv2_simple_jack_host_LDADD        = ../../src/libslv2.la @JACK_LIBS@ @RASQAL_LIBS@

slv2_simple_jack_host_SOURCES = \
	simple_jack_host.c


slv2_host_CFLAGS = @JACK_CFLAGS@ $(AM_CFLAGS)


slv2_host_DEPENDENCIES = ../../src/libslv2.la
slv2_host_LDADD        = ../../src/libslv2.la @JACK_LIBS@ @RASQAL_LIBS@

slv2_host_SOURCES = \
	host.c


endif # WITH_JACK
