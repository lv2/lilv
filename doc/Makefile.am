EXTRA_DIST = mainpage.dox reference.doxygen

all-local: doc

doc: $(srcdir)/reference.doxygen.in reference.doxygen $(srcdir)/../slv2/*.h
	doxygen reference.doxygen
