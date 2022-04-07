AM_CPPFLAGS =					\
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
