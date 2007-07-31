AUTOMAKE_OPTIONS = foreign

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

SUBDIRS      = src slv2 utils hosts data $(DOC_DIR) swig
DIST_SUBDIRS = src slv2 utils hosts data doc swig

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libslv2.pc

dist-hook: dist-check-doxygen

distclean-local:
	rm -rf doc/html
	rm -rf doc/man
	rm -rf doc/reference.doxygen
