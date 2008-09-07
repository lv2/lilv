AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = COPYING INSTALL

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

SUBDIRS      = src slv2 utils hosts $(DOC_DIR) swig debian-sid
DIST_SUBDIRS = src slv2 utils hosts doc swig debian-sid

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = slv2.pc

dist-hook: dist-check-doxygen

docs:
	cd doc && make docs

install-man:
	cd doc && make install-man

deb:
	if [ -e debian ]; then \
		echo "ERROR: ./debian exists, you must move it out of the way."; \
	else \
		ln -s debian-sid debian; \
		dpkg-buildpackage -sn; \
		rm debian; \
	fi

