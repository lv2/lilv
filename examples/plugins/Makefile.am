#AM_CFLAGS = -I$(top_srcdir)/include
#AM_LDFLAGS = -module -avoidversion -Wc,-nostartfiles
#
#plugindir = @umpdir@
#
#plugin_LTLIBRARIES = \
#	test_plugin.la
#
## Stolen from swh-plugins, makes stupid libtool versions go away
#install-pluginLTLIBRARIES: $(plugin_LTLIBRARIES)
#	mkdir -p @umpdir@
#	list='$(plugin_LTLIBRARIES)'; \
#	for file in $$list; do \
#	  sofile=`basename $$file .la`.so; \
#	  $(INSTALL_PROGRAM) .libs/$$sofile @umpdir@; \
#	done
#
#uninstall-pluginLTLIBRARIES: $(plugin_LTLIBRARIES)
#	list='$(plugin_LTLIBRARIES)'; \
#	for file in $$list; do \
#	  sofile=`basename $$file .la`.so; \
#	  rm -f @umpdir@/$$sofile; \
#	done
#
