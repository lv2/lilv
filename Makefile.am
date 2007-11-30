AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = COPYING INSTALL slv2.ttl

if HAVE_DOXYGEN
DOC_DIR = doc
dist-check-doxygen:
else
DOC_DIR =
dist-check-doxygen:
	@echo
	@echo ' ******' You need doxygen installed to make dist.' ******'
	@echo
	@false
endif

#SUBDIRS      = src slv2 utils hosts data $(DOC_DIR) swig debian
#DIST_SUBDIRS = src slv2 utils hosts data doc swig debian
SUBDIRS = src slv2 utils hosts data doc swig debian

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = slv2.pc

dist-hook: dist-check-doxygen

distclean-local:
	rm -rf doc/slv2
	rm -rf doc/reference.doxygen

docs:
	cd doc && make docs

deb:
	debuild -i --lintian-opts -i
