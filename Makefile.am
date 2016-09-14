INCLUDES =					\
	$(GTK_CFLAGS)				\
	-DG_DISABLE_DEPRECATED			\
	-DGDK_DISABLE_DEPRECATED		\
	-DGTK_DISABLE_DEPRECATED		\
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
