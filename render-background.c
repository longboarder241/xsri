/* -*- mode: C; c-file-style: "linux" -*- */

/*
 * Background display property module.
 * (C) 1997 the Free Software Foundation
 * (C) 2000 Helix Code, Inc.
 * (C) 1999, 2000, 2001,2002 Red Hat, Inc.
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
#include <gdk/gdkx.h>

#include <X11/Xatom.h>

#include <math.h>
#include <string.h> 

#include "render-background.h"

static void
fill_gradient (GdkPixbuf *pixbuf,
	       GdkColor  *c1,
	       GdkColor  *c2,
	       int        vertical,
	       int        gradient_width,
	       int        gradient_height,
	       int        pixbuf_x,
	       int        pixbuf_y)
{
	int i, j;
	int dr, dg, db;
	int gs1;
	int vc = (!vertical || (c1 == c2));
	int w = gdk_pixbuf_get_width (pixbuf);
	int h = gdk_pixbuf_get_height (pixbuf);
	guchar *b, *row;
	guchar *d = gdk_pixbuf_get_pixels (pixbuf);
	int rowstride = gdk_pixbuf_get_rowstride (pixbuf);

#define R1 c1->red
#define G1 c1->green
#define B1 c1->blue
#define R2 c2->red
#define G2 c2->green
#define B2 c2->blue

	dr = R2 - R1;
	dg = G2 - G1;
	db = B2 - B1;

	gs1 = (vertical) ? gradient_height-1 : gradient_width-1;

	row = g_new (unsigned char, rowstride);

	if (vc) {
		b = row;
		for (j = 0; j < w; j++) {
			*b++ = (R1 + ((j + pixbuf_x) * dr) / gs1) >> 8;
			*b++ = (G1 + ((j + pixbuf_x) * dg) / gs1) >> 8;
			*b++ = (B1 + ((j + pixbuf_x) * db) / gs1) >> 8;
		}
	}

	for (i = 0; i < h; i++) {
		if (!vc) {
			unsigned char cr, cg, cb;
			cr = (R1 + ((i + pixbuf_y) * dr) / gs1) >> 8;
			cg = (G1 + ((i + pixbuf_y) * dg) / gs1) >> 8;
			cb = (B1 + ((i + pixbuf_y) * db) / gs1) >> 8;
			b = row;
			for (j = 0; j < w; j++) {
				*b++ = cr;
				*b++ = cg;
				*b++ = cb;
			}
		}
		memcpy (d, row, w * 3);
		d += rowstride;
	}

#undef R1
#undef G1
#undef B1
#undef R2
#undef G2
#undef B2

	g_free (row);
}

#define RMAX  (3*1024)
#define RMAX2 (RMAX*RMAX) 

static guchar
shade_pixel (guchar pixel, double shade)
{
  if (shade > 0)
    {
      int tmp = shade * pixel * sqrt(3);
      return MIN (tmp, 255);
    }
  else
    return 0;
}

static void
emboss (GdkPixbuf *image, GdkPixbuf *boss, int x_offset, int y_offset)
{
  int image_width = gdk_pixbuf_get_width (image);
  int image_height = gdk_pixbuf_get_height (image);
  int boss_width = gdk_pixbuf_get_width (boss);
  int boss_height = gdk_pixbuf_get_height (boss);
  int i,j;

  /* Average the three channels of the boss image */
  
  gushort *boss_gray = g_new (gushort, boss_width * boss_height);

  for (j=0; j<boss_height; j++)
    {
      guchar *pixels = gdk_pixbuf_get_pixels (boss) + j * gdk_pixbuf_get_rowstride (boss);
      gushort *line = boss_gray + boss_width * j;

      for (i=0; i<boss_width; i++)
        {
          line[i] = pixels[0] + pixels[1] + pixels[2];
	  pixels += 3;    
	}
    }

  for (j=1; j<boss_height - 1; j++)
    {
      if (j + y_offset >= 0 && j + y_offset < image_height)
	{
	  guchar *p = gdk_pixbuf_get_pixels (image) + (j + y_offset) * gdk_pixbuf_get_rowstride (image) + 3 * x_offset;
      
	  for (i=1; i<boss_width - 1; i++)
	    {
	      double shade;
	      
	      if (i + x_offset >= 0 && i + x_offset < image_width)
		{
		  int dx = (*(boss_gray + boss_width * j +       i + 1) -
			    *(boss_gray + boss_width * j +       i - 1));
		  int dy = (*(boss_gray + boss_width * (j - 1) + i) -
			    *(boss_gray + boss_width * (j + 1) + i));
		  int dz;
		  
		  int t = dx*dx + dy*dy;

		  if (t > RMAX2)
		    {
		      double sqrt_t = sqrt(t);
		      
		      dz = 0;
		      dx = RMAX * dx / sqrt_t;
		      dy = RMAX * dy / sqrt_t;
		    }
		  else
		    {
		      dz = sqrt (RMAX2 - t);
		    }
	      
		  shade = (double)(dz - dx + dy) / (RMAX * sqrt(3));
		  
		  *(p) = shade_pixel (*p, shade);
		  *(p+1) = shade_pixel (*(p+1), shade);
		  *(p+2) = shade_pixel (*(p+2), shade);
		}
	      p += 3;
	    }

	}
    }

  g_free (boss_gray);
}

/* Create a persistant pixmap. We create a separate display
 * and set the closedown mode on it to RetainPermanent
 */
GdkPixmap *
make_root_pixmap (gint width, gint height)
{
	Pixmap result;
	Display *display;

	gdk_flush ();

	display = XOpenDisplay (gdk_get_display ());
	XSetCloseDownMode (display, RetainPermanent);

	result = XCreatePixmap (display,
				DefaultRootWindow (display),
				width, height,
				DefaultDepthOfScreen (DefaultScreenOfDisplay (GDK_DISPLAY())));
	XCloseDisplay (display);

	return gdk_pixmap_foreign_new (result);
}

void
dispose_root_pixmap (GdkPixmap *pixmap)
{
	g_object_unref (pixmap);
}

/* Set the root pixmap, and properties pointing to it. We
 * do this atomically with XGrabServer to make sure that
 * we won't leak the pixmap if somebody else it setting
 * it at the same time. (This assumes that they follow the
 * same conventions we do
 */
void 
set_root_pixmap (GdkPixmap *pixmap)
{
	Atom type;
	gulong nitems, bytes_after;
	gint format;
	guchar *data_esetroot;
	Pixmap pixmap_id;
	int result;

	XGrabServer (GDK_DISPLAY());

	result = XGetWindowProperty (GDK_DISPLAY(), get_root_xwindow(),
				     gdk_x11_get_xatom_by_name("ESETROOT_PMAP_ID"),
				     0L, 1L, False, XA_PIXMAP,
				     &type, &format, &nitems, &bytes_after,
				     &data_esetroot);

	if (result == Success && type == XA_PIXMAP && format == 32 && nitems == 1) {
		XKillClient(GDK_DISPLAY(), *(Pixmap*)data_esetroot);
	}

	if (data_esetroot != NULL) {
		XFree (data_esetroot);
	}

	if (pixmap != NULL) {
		pixmap_id = GDK_WINDOW_XWINDOW (pixmap);

		XChangeProperty (GDK_DISPLAY(), get_root_xwindow(),
				 gdk_x11_get_xatom_by_name("ESETROOT_PMAP_ID"), 
				 XA_PIXMAP, 32, PropModeReplace,
				 (guchar *) &pixmap_id, 1);
		XChangeProperty (GDK_DISPLAY(), get_root_xwindow(),
				 gdk_x11_get_xatom_by_name("_XROOTPMAP_ID"), 
				 XA_PIXMAP, 32, PropModeReplace,
				 (guchar *) &pixmap_id, 1);

		XSetWindowBackgroundPixmap (GDK_DISPLAY(), get_root_xwindow(), 
					    pixmap_id);
	} else {
		XDeleteProperty (GDK_DISPLAY (), get_root_xwindow (),
				 gdk_x11_get_xatom_by_name("ESETROOT_PMAP_ID"));
		XDeleteProperty (GDK_DISPLAY(), get_root_xwindow(),
				 gdk_x11_get_xatom_by_name("_XROOTPMAP_ID"));
	}

	XClearWindow (GDK_DISPLAY (), get_root_xwindow ());
	XUngrabServer (GDK_DISPLAY());

	XFlush(GDK_DISPLAY());
}

gulong 
xpixel_from_color (GdkColor *color)
{
	GdkColormap *colormap = gdk_colormap_get_system ();
	GdkColor tmp_color = *color;

	gdk_rgb_find_color (colormap, &tmp_color);

	return tmp_color.pixel;
}

void
xpixel_to_color (gulong pixel, GdkColor *color)
{
	XColor xcolor;
	Colormap cmap;

	cmap = GDK_COLORMAP_XCOLORMAP (gdk_colormap_get_system ());

	xcolor.pixel = pixel;
	XQueryColor (GDK_DISPLAY(), cmap, &xcolor);

	color->red = xcolor.red;
	color->green = xcolor.green;
	color->blue = xcolor.blue;
}

static void 
composite (GdkPixbuf       *dest,
	   int              dest_x,
	   int              dest_y,
	   GdkPixbuf       *src,
	   int              at_x,
	   int              at_y,
	   int              at_width,
	   int              at_height,
	   int              overall_alpha)
{
	GdkRectangle emblem_rect;
	GdkRectangle pixbuf_rect;
	GdkRectangle dest_rect;
	
	emblem_rect.x = at_x;
	emblem_rect.y = at_y;
	emblem_rect.width = at_width;
	emblem_rect.height = at_height;
	
	pixbuf_rect.x = dest_x;
	pixbuf_rect.y = dest_y;
	pixbuf_rect.width = gdk_pixbuf_get_width (dest);
	pixbuf_rect.height = gdk_pixbuf_get_height (dest);
	
	gdk_rectangle_intersect (&emblem_rect, &pixbuf_rect, &dest_rect);
	
	gdk_pixbuf_composite (src, dest,
			      dest_rect.x - dest_x, dest_rect.y - dest_y, dest_rect.width, dest_rect.height,
			      at_x - dest_x, at_y - dest_y,
			      (double)at_width / (gdk_pixbuf_get_width (src)),
			      (double)at_height / (gdk_pixbuf_get_height (src)),
			      GDK_INTERP_BILINEAR, overall_alpha);
}

static void
get_visibility (BGState     *state,
		gboolean     tile_only,
		gint         x,
		gint         y,
		gint         width,
		gint         height,
		gboolean    *see_colors,
		gboolean    *see_tiles,
		gboolean    *see_emblem)
{
	*see_colors = TRUE;
	*see_tiles = state->tile_pixbuf != NULL;
	*see_emblem = !tile_only && state->emblem_pixbuf != NULL;

	if (*see_tiles && !gdk_pixbuf_get_has_alpha (state->tile_pixbuf) && state->tile_alpha == 255)
		*see_colors = FALSE;

	if (*see_emblem) {
		if (!gdk_pixbuf_get_has_alpha (state->emblem_pixbuf) &&
		    state->emblem_alpha == 255 &&
		    x >= state->emblem_x &&
		    x + width <= state->emblem_x + state->emblem_width &&
		    y >= state->emblem_y &&
		    y + height <= state->emblem_y + state->emblem_height) {
			*see_colors = FALSE;
			*see_tiles = FALSE;
		}

		if (state->emblem_x + state->emblem_width <= x ||
		    state->emblem_x >= x + width ||
		    state->emblem_y + state->emblem_height < y ||
		    state->emblem_y >= y + height) {
			*see_emblem = FALSE;
		}
	}
}

static int
gcd (int a, int b)
{
	if (a < b)
		goto b_greater;
	
	while (TRUE) {
		a = a % b;
		if (a == 0)
			return b;
	b_greater:
		b = b % a;
		if (b == 0)
			return a;
	}
}

static int
lcm (int a, int b)
{
	return (a / gcd (a, b)) * b;

}

gboolean
background_get_tile_size (BGState *state,
			  int     *width_return,
			  int     *height_return)
{
	gboolean see_colors, see_tiles, see_emblem;
	gint width;
	gint height;
	
	get_visibility (state, FALSE, 0, 0, state->width, state->height, &see_colors, &see_tiles, &see_emblem);

	if (!see_colors && !see_tiles)
		return FALSE;

	width = 1;
	height = 1;

	if (see_colors) {
		GdkVisual *visual;
		gboolean dithered;
		
		visual = gdk_visual_get_system ();

		if ((visual->type == GDK_VISUAL_TRUE_COLOR ||
		     visual->type == GDK_VISUAL_DIRECT_COLOR) &&
		    visual->depth == 24) {
			dithered = FALSE;
		} else {
			dithered = TRUE;
		}

		width = dithered ? 128 : 1;
		height = dithered ? 128 : 1;

		if (state->grad) {
			if (state->vertical) {
				height = state->height;
			} else {
				width = state->width;
			}
		}
	}

	if (see_tiles && state->tile_pixbuf) {
		width = MIN (lcm (width, state->tile_width),
			     state->width);
		height = MIN (lcm (height, state->tile_height),
			      state->height);
	}

	if (width == state->width && height == state->height)
		return FALSE;
	
	if (width_return)
		*width_return = width;
	if (height_return)
		*height_return = height;

	return TRUE;
}

void
background_render (BGState     *state,
		   GdkDrawable *drawable,
		   gboolean     tile_only,
		   gint         x, 
		   gint         y,
		   gint         width,
		   gint         height)
{
	GdkGC *gc;
	
	gboolean see_colors, see_tiles, see_emblem;
	GdkPixbuf *pixbuf;

	get_visibility (state, tile_only, x, y, width, height, &see_colors, &see_tiles, &see_emblem);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);

	if (see_colors) {
		if (state->grad)
			fill_gradient (pixbuf, &state->bgColor1, &state->bgColor2, state->vertical,
				       state->width, state->height, x, y);
		else
			fill_gradient (pixbuf, &state->bgColor1, &state->bgColor1, FALSE,
				       state->width, state->height, x, y);
	}

	if (see_tiles) {
		int xoff, yoff;

		int xoff_start = x - x % state->tile_width;
		int yoff_start = y - y % state->tile_height;

		for (yoff = yoff_start; yoff < height; yoff += state->tile_height)
			for (xoff = xoff_start; xoff < width; xoff += state->tile_width) {
				composite (pixbuf, x, y, state->tile_pixbuf,
					   xoff, yoff, state->tile_width, state->tile_height,
					   state->tile_alpha);
			}
	}

	if (see_emblem) {
		if (state->emboss) {
			GdkPixbuf *boss;
			int image_width = gdk_pixbuf_get_width (state->emblem_pixbuf);
			int image_height = gdk_pixbuf_get_height (state->emblem_pixbuf);

			if (image_width == state->emblem_width &&
			    image_height == state->emblem_height &&
			    !gdk_pixbuf_get_has_alpha (state->emblem_pixbuf)) {
				boss = gdk_pixbuf_ref (state->emblem_pixbuf);
			} else {
				boss = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
						       state->emblem_width, state->emblem_height);
				
				gdk_pixbuf_composite_color (state->emblem_pixbuf, boss,
							    0, 0, state->emblem_width, state->emblem_height,
							    0, 0,
							    (double)state->emblem_width / image_width,
							    (double)state->emblem_height / image_height,
							    GDK_INTERP_BILINEAR,
							    255, 0, 0, 16,
							    0xffffff, 0xffffff);
			}

			emboss (pixbuf, boss, state->emblem_x - x, state->emblem_y - y);
			gdk_pixbuf_unref (boss);
		} else {
			composite (pixbuf, x, y, state->emblem_pixbuf,
				   state->emblem_x, state->emblem_y, state->emblem_width, state->emblem_height,
				   state->emblem_alpha);
		}
	}
								     
								      
	
	gc = gdk_gc_new (drawable);
	gdk_pixbuf_render_to_drawable (pixbuf, drawable, gc,
				       0, 0, 0, 0, width, height,
				       GDK_RGB_DITHER_MAX, x, y);
	g_object_unref (gc);

	gdk_pixbuf_unref (pixbuf);
}

/* We abstract the functionality of getting the root window since xscreensaver
 * likes to use a background setting program to set the background of it's
 * own fake root window.
 */
Window
get_root_xwindow (void)
{
	static Window root_window = None;

	if (root_window == None) {
		Window root_return, parent;
		Window *children;
		Atom type;
		unsigned int n_children;
		unsigned int i;
		
		root_window = GDK_ROOT_WINDOW ();

		XQueryTree (GDK_DISPLAY (), GDK_ROOT_WINDOW (),
			    &root_return, &parent, &children, &n_children);

		gdk_error_trap_push ();

		for (i = 0; i < n_children; i++) {
			Window *data;
			int format;
			unsigned long n_items, bytes_after;
			
			if (XGetWindowProperty (GDK_DISPLAY (), children[i],
						gdk_x11_get_xatom_by_name ("__SWM_VROOT"),
						0, 1, False, XA_WINDOW,
						&type, &format, &n_items, &bytes_after, (guchar **)&data) == Success) {
				if (type != None) {
					if (format == 32 && n_items == 1 && bytes_after == 0) {
						XFree (data);
						root_window = *data;
						break;
					} else {
						XFree (data);
					}
				}
			}
						
					    
		}
		
		gdk_error_trap_pop ();

		XFree (children);
	}

	return root_window;
}

GdkWindow *
get_root_gdk_window (void)
{
	static GdkWindow *root_window = NULL;

	if (!root_window) {
		Window root_xwindow = get_root_xwindow ();

		root_window = gdk_window_lookup (root_xwindow);
		if (!root_window)
			root_window = gdk_window_foreign_new (root_xwindow);
	}

	return root_window;
}

