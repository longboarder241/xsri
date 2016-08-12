/* -*- mode: C; c-file-style: "linux" -*- */

/*
 * Background display property module.
 * (C) 1997 the Free Software Foundation
 * (C) 2000 Helix Code, Inc.
 * (C) 1999, 2000, 2001 Red Hat, Inc.
 *
 * Authors: Miguel de Icaza.
 *          Federico Mena.
 *          Radek Doulik
 *          Michael Fulbright
 *          Justin Maurer
 *          Owen Taylor
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

typedef struct _BGState BGState;

struct _BGState {
	gint       width;
	gint       height;
	GdkColor   bgColor1, bgColor2;
	gboolean   grad;
	gboolean   emboss;
	gboolean   vertical;
	GdkPixbuf *tile_pixbuf;
	gint       tile_alpha;
	gint       tile_width;
	gint       tile_height;
	GdkPixbuf *emblem_pixbuf;
	gint       emblem_alpha;
	gint       emblem_x;
	gint       emblem_y;
	gint       emblem_width;
	gint       emblem_height;
};

GdkPixmap *make_root_pixmap (gint width, gint height);
void       dispose_root_pixmap (GdkPixmap *pixmap);
void       set_root_pixmap (GdkPixmap *pixmap);

void render_to_drawable (GdkPixbuf *pixbuf,
			 GdkDrawable *drawable, GdkGC *gc,
			 int src_x, int src_y,
			 int dest_x, int dest_y,
			 int width, int height,
			 GdkRgbDither dither,
			 int x_dither, int y_dither);

gulong xpixel_from_color (GdkColor *color);
void   xpixel_to_color (gulong pixel, GdkColor *color);

void     background_render        (BGState     *state,
				   GdkDrawable *drawable,
				   gboolean     tile_only,
				   gint         x,
				   gint         y,
				   gint         width,
				   gint         height);
gboolean background_get_tile_size (BGState     *state,
				   int         *width_return,
				   int         *height_return);

GdkWindow *get_root_gdk_window (void);
Window     get_root_xwindow    (void);
