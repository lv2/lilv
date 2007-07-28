EXTRA_DIST = mainpage.dox reference.doxygen

all-local: doc

doc: reference.doxygen $(top_srcdir)/slv2/*.h
	doxygen reference.doxygen
