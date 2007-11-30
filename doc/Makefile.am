EXTRA_DIST = mainpage.dox man.dox html.dox reference.doxygen

man_MANS = \
	@abs_top_builddir@/doc/slv2/man/man3/slv2.3 \
	@abs_top_builddir@/doc/slv2/man/man3/slv2_collections.3 \
	@abs_top_builddir@/doc/slv2/man/man3/slv2_data.3 \
	@abs_top_builddir@/doc/slv2/man/man3/slv2_library.3 \
	@abs_top_builddir@/doc/slv2/man/man3/slv2_util.3 \
	@abs_top_builddir@/doc/slv2/man/man3/slv2_world.3

all-local: docs

docs: $(srcdir)/reference.doxygen.in reference.doxygen $(srcdir)/../slv2/*.h
	mkdir -p @abs_top_builddir@/doc/slv2
	doxygen reference.doxygen
