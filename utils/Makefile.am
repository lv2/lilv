AM_CFLAGS = -I$(top_srcdir) @RASQAL_CFLAGS@

bin_PROGRAMS = lv2_list lv2_inspect

lv2_list_CFLAGS = "-std=c99"
lv2_list_SOURCES = lv2_list.c
lv2_list_LDADD = ../src/libslv2.la 

lv2_inspect_CFLAGS = "-std=c99"
lv2_inspect_SOURCES = lv2_inspect.c
lv2_inspect_LDADD = ../src/libslv2.la 

if BUILD_LADSPA2SLV2

bin_PROGRAMS += ladspa2lv2

ladspa2lv2_CXXFLAGS = @RAUL_CFLAGS@ @RAPTOR_CFLAGS@
ladspa2lv2_LDADD = -ldl @RAUL_LIBS@ @RAPTOR_LIBS@
ladspa2lv2_SOURCES = ladspa2lv2.cpp

endif
