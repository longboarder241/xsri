INCLUDES =					\
	$(GTK_CFLAGS)				\
	-DSYSCONFDIR=\"$(sysconfdir)\"

bin_PROGRAMS = xsri

xsri_SOURCES =					\
	render-background.c			\
	render-background.h			\
	xsri.c

xsri_LDADD =					\
	$(IMLIB_LIBS)				\
	$(GTK_LIBS)				\
	-lpopt -lm -lX11

EXTRA_DIST =					\
	xsri.spec.in

dist-hook: xsri.spec
	if grep '^- Version @VERSION@' $(srcdir)/xsri.spec.in ; then : ; else \
	   perl -ni.bak -e \
	      'print; if (/^%changelog/) { chomp ($$date = `date +"%a %b %d %Y"`); print "* $$date Owen Taylor <otaylor\@redhat.com>\n- Version @VERSION@\n\n"; }' \
	      $(srcdir)/xsri.spec.in ; \
	   ( CONFIG_FILES=xsri.spec CONFIG_HEADERS= $(SHELL) ./config.status ) ; \
	   cp -p $(srcdir)/xsri.spec.in $(distdir) ; \
	fi
	cp xsri.spec $(distdir)

