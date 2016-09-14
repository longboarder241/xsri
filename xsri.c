/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <popt.h>
#include <stdarg.h>

#include "config.h"

#include "render-background.h"

typedef enum {
        RUN_MODE_TEST,
        RUN_MODE_SET,
        RUN_MODE_RUN
} RunMode;

static char *appname;

static int want_my_version = FALSE;
  
int run_mode = RUN_MODE_SET;

static const char *color = "#356390";
static const char *color2 = NULL;
static const char *geometry = NULL;
static const char *tile_file = NULL;
static const char *emblem_file = NULL;
static int emboss = FALSE;
static int vertical = TRUE;
static int center_x = FALSE;
static int center_y = FALSE;
static const char *scale_width = NULL;
static const char *scale_height = NULL;
static const char *avoid = NULL;
static int keep_aspect = FALSE;
static int debug = FALSE;
static int dummy = FALSE;
static int tile_alpha = 255;
static int emblem_alpha = 255;

static const struct poptOption options_table[] = {
        { "version", 0, POPT_ARG_NONE, &want_my_version, 0,
          "output version of xsri" },
        { "color", 0, POPT_ARG_STRING, &color, 0,
          "background color", "COLOR" },
        { "color2", 0, POPT_ARG_STRING, &color2, 0,
          "second background color (forms a gradient)", "COLOR" },
        { "hgradient", 0, POPT_ARG_VAL, &vertical, FALSE,
          "use a horizontal gradient" },
        { "vgradient", 0, POPT_ARG_VAL, &vertical, TRUE,
          "use a vertical gradient (default)" },
        { "tile", 0, POPT_ARG_STRING, &tile_file, 0,
          "image file to tile across screen", "FILE" },
        { "tile-alpha", 0, POPT_ARG_INT, &tile_alpha, 0,
          "alpha value for tile image", "0-255" },
        { "emblem", 0, POPT_ARG_STRING, &emblem_file, 0,
          "image file to place on top of the gradient and tile", "FILE" },
        { "emblem-alpha", 0, POPT_ARG_INT, &emblem_alpha, 0,
          "alpha value for emblem image", "0-255" },
        { "emboss", 0, POPT_ARG_NONE, &emboss, 0,
          "emboss emblem" },
        { "geometry", 0, POPT_ARG_STRING, &geometry, 0,
          "location and/or size of emblem", "WIDTHxHEIGHT+X+Y" },
        { "center-x", 0, POPT_ARG_NONE, &center_x, 0,
          "center emblem in the X direction" },
        { "center-y", 0, POPT_ARG_NONE, &center_y, 0,
          "center emblem in the Y direction" },
        { "scale-width", 0, POPT_ARG_STRING, &scale_width, 0,
          "scale width of emblem to percentage of screen width", "PERCENTAGE" },
        { "scale-height", 0, POPT_ARG_STRING, &scale_height, 0,
          "scale height of emblem to percentage of screen height", "PERCENTAGE" },
        { "avoid", 0, POPT_ARG_STRING, &avoid, 0,
          "rectangle to avoid. Emblem is shrunk to achieve this", "WIDTHxHEIGHT+X+Y" },
        { "keep-aspect", 0, POPT_ARG_NONE, &keep_aspect, 0,
          "shrink width or height of emblem to maintain aspect ratio" },
        { "set", 0, POPT_ARG_VAL, &run_mode, RUN_MODE_SET,
          "set the desktop background image" },
        { "run", 0, POPT_ARG_VAL, &run_mode, RUN_MODE_RUN,
          "set background and stick around" },
        { "test", 0, POPT_ARG_VAL, &run_mode, RUN_MODE_TEST,
          "show background in a window" },
        { "debug", 0, POPT_ARG_NONE | POPT_ARGFLAG_DOC_HIDDEN, &debug, 0, NULL },
        { "dummy", 0, POPT_ARG_NONE | POPT_ARGFLAG_DOC_HIDDEN, &dummy, 0, NULL },
        POPT_AUTOHELP
        { NULL, 0, 0, NULL, 0 }
};

static BGState bg_state;

static void
debugmsg (char *format, ...)
{
        if (debug) {
                va_list va;
                
                va_start (va, format);
                vfprintf (stderr, format, va);
                va_end (va);
        }
}

static void
load_images (void)
{
        GError *error = NULL;
        
        if (tile_file) {
                bg_state.tile_pixbuf = gdk_pixbuf_new_from_file (tile_file, &error);
                if (!bg_state.tile_pixbuf) {
                        fprintf (stderr, "%s: Cannot load tile image: %s: %s\n",
                                 appname, tile_file, error->message);
                
                        g_error_free (error);
                } else {
                        bg_state.tile_width = gdk_pixbuf_get_width (bg_state.tile_pixbuf);
                        bg_state.tile_height = gdk_pixbuf_get_height (bg_state.tile_pixbuf);
                }
        } else
                bg_state.tile_pixbuf = NULL;

        if (tile_alpha < 0 || tile_alpha > 255) {
                fprintf (stderr, "%s: Invalid alpha value %d\n", appname, tile_alpha);
                exit (1);
        }

        bg_state.tile_alpha = tile_alpha;
        
        if (emblem_file) {
                bg_state.emblem_pixbuf = gdk_pixbuf_new_from_file (emblem_file, &error);
                if (!bg_state.emblem_pixbuf) {
                        fprintf (stderr, "%s: Cannot load emblem image: %s: %s\n",
                                 appname, emblem_file, error->message);
                        g_error_free (error);
                }
        } else
                bg_state.emblem_pixbuf = NULL;
        
        if (emblem_alpha < 0 || emblem_alpha > 255) {
                fprintf (stderr, "%s: Invalid emblem alpha value %d\n", appname, emblem_alpha);
                exit (1);
        }

        bg_state.emblem_alpha = emblem_alpha;
        
        bg_state.emboss = emboss;
}

static void
parse_colors (void)
{
        bg_state.vertical = vertical;
        
        if (color) {
                if (!gdk_color_parse (color, &bg_state.bgColor1))
                        fprintf (stderr, "%s: Cannot parse_color: %s\n", appname, color);
        }
        if (color2) {
                if (!gdk_color_parse (color2, &bg_state.bgColor2))
                        fprintf (stderr, "%s: Cannot parse_color: %s\n", appname, color);
                else
                        bg_state.grad = TRUE;
        }

        /* The following code tries to coerce colors to solid colors when close, but
         * there is some inaccuracy somewhere along the way, and it actually doesnt
         * quite work.
         */
#if 0        
        if (!bg_state.grad) {
                GdkColor tmp_color;
                gint diff;
                
                gulong pixel = xpixel_from_color (&bg_state.bgColor1);
                xpixel_to_color (pixel, &tmp_color);

                diff = 0;
                diff = MAX (diff, abs (tmp_color.red - bg_state.bgColor1.red));
                diff = MAX (diff, abs (tmp_color.green - bg_state.bgColor1.green));
                diff = MAX (diff, abs (tmp_color.blue - bg_state.bgColor1.blue));

                if (diff < 8 * 0x101)
                        bg_state.bgColor1 = tmp_color;
        }
#endif
}

static void
position_emblem (void)
{
        /* See README for a description of the algorithm
         */
        int width, height;
        int x, y;
        int gravity_x, gravity_y;
        int screen_width = gdk_screen_width ();
        int screen_height = gdk_screen_height ();
        int image_width = gdk_pixbuf_get_width (bg_state.emblem_pixbuf);
        int image_height = gdk_pixbuf_get_height (bg_state.emblem_pixbuf);
        int geometry_flags = 0;
        int geometry_x = 0;
        int geometry_y = 0;
        unsigned int geometry_width = 0;
        unsigned int geometry_height = 0;

        if (geometry) {
                geometry_flags = XParseGeometry (geometry, &geometry_x, &geometry_y,
                                                 &geometry_width, &geometry_height);

                if (((geometry_flags & WidthValue) && geometry_width == 0) ||
                    ((geometry_flags & HeightValue) && geometry_height == 0)) {
                        fprintf (stderr, "%s: Invalid geometry specification: %s\n",
                                 appname, geometry);
                        exit (1);
                }
        }

        if (scale_width) {
                char *p;
                double percentage = strtod (scale_width, &p);

                if (*p || percentage < 0 || percentage > 100) {
                        fprintf (stderr, "%s: Invalid percentage: %s\n", appname, scale_width);
                        exit (1);
                }

                width = 0.5 + screen_width * (percentage / 100.);
        } else if (geometry_flags & WidthValue)
                width = geometry_width;
        else
                width = image_width;


        if (scale_height) {
                char *p;
                double percentage = strtod (scale_height, &p);

                if (*p || percentage < 0 || percentage > 100) {
                        fprintf (stderr, "%s: Invalid percentage: %s\n", appname, scale_height);
                        exit (1);
                }

                height = 0.5 + screen_height * (percentage / 100.);
        } else if (geometry_flags & HeightValue)
                height = geometry_height;
        else
                height = image_height;

        if (center_x) {
                x = (screen_width - width) / 2;
                gravity_x = 0;
        } else if (geometry_flags & XValue) {
                if (geometry_flags & XNegative) {
                        x = screen_width - width + geometry_x;
                        gravity_x = 1;
                } else {
                        x = geometry_x;
                        gravity_x = -1;
                }
        } else {
                x = 0;
                gravity_x = -1;
        }
        
        if (center_y) {
                y = (screen_height - height) / 2;
                gravity_y = 0;
        }  else if (geometry_flags & YValue) {
                if (geometry_flags & YNegative) {
                        y = screen_height - height + geometry_y;
                        gravity_y = 1;
                } else {
                        y = geometry_y;
                        gravity_y = -1;
                }
        } else {
                y = 0;
                gravity_y = -1;
        }
        
        if (avoid || keep_aspect) {
                if (width * image_height > image_width * height) {
                        int new_width = (height * image_width) / image_height;
                        if (gravity_x == 0) {
                                x += (width - new_width) / 2;
                        } else if (gravity_x == 1) {
                                x += (width - new_width);
                        }
                        width = new_width;
                } else if (width * image_height < image_width * height) {
                        int new_height = (width * image_height) / image_width;
                        if (gravity_y == 0) {
                                y += (height - new_height) / 2;
                        } else if (gravity_y == 1) {
                                y += (height - new_height);
                        }
                        height = new_height;
                }
        }

        if (avoid) {
                int avoid_x, avoid_y, avoid_width, avoid_height;
                int avoid_flags;
                int new_width, new_height;
                int tmp_x, tmp_y;
                int x_space, y_space;
                
                avoid_flags = XParseGeometry (avoid, &avoid_x, &avoid_y,
                                              &avoid_width, &avoid_height);

                if (!(avoid_flags & WidthValue) || avoid_width == 0 ||
                    !(avoid_flags & HeightValue) || avoid_height == 0) {
                        fprintf (stderr, "%s: avoid geometry '%s' must have positive width, height\n", appname, avoid);
                        exit (1);
                }

                if (!(avoid_flags & XValue))
                        avoid_x = (screen_width - avoid_width) / 2;
                else if (avoid_flags & XNegative)
                        avoid_x = screen_width - avoid_width + avoid_x;
                
                if (!(avoid_flags & YValue))
                        avoid_y = (screen_height - avoid_height) / 2;
                else if (avoid_flags & YNegative)
                        avoid_y = screen_height - avoid_height + avoid_y;

                /* To avoid worrying about gravity, we flip horizontally
                 * and/or vertically to do the following computation
                 */
                if (gravity_x == 1 ||
                    (gravity_x == 0 && x + width / 2 > avoid_x + avoid_width)) {
                        tmp_x = screen_width - width - x;
                        avoid_x = screen_width - avoid_width - avoid_x;
                } else
                        tmp_x = x;
                
                if (gravity_y == 1 ||
                    (gravity_y == 0 && y + height / 2 > avoid_y + avoid_height)) {
                        tmp_y = screen_height - height - y;
                        avoid_y = screen_height - avoid_height - avoid_y;
                } else
                        tmp_y = y;

                debugmsg ("Now: %dx%d+%d+%d\n", width, height, tmp_x, tmp_y);
                debugmsg ("Avoiding: %dx%d+%d+%d\n", avoid_width, avoid_height, avoid_x, avoid_y);

                if ((tmp_x + width < avoid_x || tmp_x >= avoid_x + avoid_width ||
                     tmp_y + height < avoid_y || tmp_y >= avoid_y + avoid_height)) {
                        /* No need to adjust */

                        goto avoid_done;
                } 

                if (gravity_x == 0)
                        x_space = 2 * (avoid_x - (tmp_x + width / 2));
                else
                        x_space = avoid_x - tmp_x;
                
                if (gravity_y == 0)
                        y_space = 2 * (avoid_y - (tmp_y + width / 2));
                else
                        y_space = avoid_y - tmp_y;

                if (x_space <= 0 && y_space <= 0) {
                        /* Can't adjust */
                        
                        g_object_unref (bg_state.emblem_pixbuf);
                        bg_state.emblem_pixbuf = NULL;

                        return;
                } 

                debugmsg ("x_space = %d, y_space = %d\n", x_space, y_space);
                
                if (x_space * image_height > y_space * image_width) {
                        new_width = x_space;
                        new_height = (image_height * x_space) / image_width;
                } else {
                        new_height = y_space;
                        new_width = (image_width * y_space) / image_height;
                }
                
                /* Now adjust width/height in our original coordinate system
                 */
                if (gravity_x == 0) {
                        x += (width - new_width) / 2;
                } else if (gravity_x == 1) {
                        x += (width - new_width);
                }

                if (gravity_y == 0) {
                        y += (height - new_height) / 2;
                } else if (gravity_y == 1) {
                        y += (height - new_height);
                }

                width = new_width;
                height = new_height;
        }
        avoid_done:

        debugmsg ("positioned at %dx%d+%d+%d: \n", width, height, x, y);
        
        bg_state.emblem_x = x;
        bg_state.emblem_y = y;
        bg_state.emblem_width = width;
        bg_state.emblem_height = height;
}

int
main (int argc, char **argv)
{
        poptContext opt_context;
        int result;
        const char *arg;
        int i;
        gchar *userrc;

        appname = g_path_get_basename (argv[0]);
  
        /* Gross hack for compatibility with old xsri */

        for (i = 1; i < argc; i++) {
                if (strcmp (argv[i], "-geometry") == 0) {
                        argv[i] = "--geometry";
                } else if (strcmp (argv[i], "-root") == 0) {
                        argv[i] = "--set";
                } else if (strcmp (argv[i], "-scale") == 0) {
                        argv[i] = "--dummy";
                        scale_width = "100";
                        scale_height = "100";
                } else if (strcmp (argv[i], "-scale-width") == 0) {
                        argv[i] = "--dummy";
                        scale_width = "100";
                } else if (strcmp (argv[i], "-scale-height") == 0) {
                        argv[i] = "--dummy";
                        scale_height = "100";
                } else if (strcmp (argv[i], "-avoid") == 0) {
                        argv[i] = "--avoid";
                } else if (strcmp (argv[i], "-center-horizontal") == 0) {
                        argv[i] = "--center-x";
                } else if (strcmp (argv[i], "-center-vertical") == 0) {
                        argv[i] = "--center-y";
                } else if (strcmp (argv[i], "-integer-scale") == 0) {
                        argv[i] = "--dummy";
                } else if (strcmp (argv[i], "-keep-aspect") == 0) { 
                        argv[i] = "--keep-aspect";
                }
        }

        gtk_init (&argc, &argv);
  
        opt_context = poptGetContext (appname, argc, (const char **)argv,
                                      options_table, 0);

        userrc = g_strconcat (g_get_home_dir (), "/.xsrirc", NULL);
        poptReadConfigFile (opt_context, userrc);
        poptReadConfigFile (opt_context, SYSCONFDIR "/X11/xsrirc");
                            
        result = poptGetNextOpt (opt_context);
        if (result != -1) {
                fprintf(stderr, "%s: %s: %s\n",
                        appname,
                        poptBadOption(opt_context, POPT_BADOPTION_NOALIAS),
                        poptStrerror(result));
                return 1;
        }

        arg = poptGetArg (opt_context);
        if (arg) {
                if (!emblem_file) {
                        emblem_file = arg;
                        arg = poptGetArg (opt_context);
                }
        }

        if (arg) {
                poptPrintUsage (opt_context, stderr, 0);
                return 1;
        }

        if (want_my_version) {
                printf ("xsri version %s\n", VERSION);
                return 0;
        }

        parse_colors ();
        load_images ();
        
        if (bg_state.emblem_pixbuf)
                position_emblem ();

        bg_state.width =  gdk_screen_width();
        bg_state.height = gdk_screen_height();

        if (run_mode == RUN_MODE_SET) {
                int tile_width, tile_height;
                GdkPixmap *pixmap;

                if (bg_state.emblem_pixbuf ||
                    !background_get_tile_size (&bg_state, &tile_width, &tile_height)) {
                        tile_width = bg_state.width;
                        tile_height = bg_state.height;
                }

                /* We could set_root_color if tile_width == 1 && tile_height == 1),
                 * but transparent-terminal apps need the pixmap. Could
                 * use set_root_color and set the pixmap...
                 */
                pixmap = make_root_pixmap (tile_width, tile_height);
                background_render (&bg_state, pixmap, FALSE,
                                   0, 0, tile_width, tile_height);
                set_root_pixmap (pixmap);
                dispose_root_pixmap (pixmap);
        }

        if (run_mode == RUN_MODE_RUN) {
                int tile_width, tile_height;
                gboolean tiles_useful = FALSE;
                GdkPixmap *pixmap;

                if (background_get_tile_size (&bg_state, &tile_width, &tile_height)) {
                        guint tile_pixels = tile_width * tile_height;
                        guint emblem_pixels = bg_state.emblem_width * bg_state.emblem_height;
                        guint all_pixels = bg_state.width * bg_state.height;

                        if (tile_pixels + emblem_pixels < all_pixels) {
                                tiles_useful = TRUE;
                                debugmsg ("Saved %d/%d (%2.0f%%) pixels by tiling\n",
                                          all_pixels - (tile_pixels + emblem_pixels), all_pixels,
                                          100 * (double)(all_pixels - (tile_pixels + emblem_pixels)) /all_pixels);
                        }
                }
                
                if (!tiles_useful) {
                        tile_width = bg_state.width;
                        tile_height = bg_state.height;
                }

                if (tile_width == 1 && tile_height == 1) {
                        XSetWindowBackground (GDK_DISPLAY(), get_root_xwindow(),
                                              xpixel_from_color (&bg_state.bgColor1));
                        XClearWindow (GDK_DISPLAY (), get_root_xwindow ());
                } else {
                        pixmap = gdk_pixmap_new (get_root_gdk_window (), tile_width, tile_height, -1);
                        background_render (&bg_state, pixmap, tiles_useful,
                                           0, 0, tile_width, tile_height);
                        
                        gdk_window_set_back_pixmap (get_root_gdk_window (), pixmap, FALSE);
                        g_object_unref (pixmap);
                        gdk_window_clear (get_root_gdk_window ());
                }
                
                if (tiles_useful && bg_state.emblem_pixbuf) {
                        GdkWindow *window;
                        
                        GdkWindowAttr attributes;
                            
                        attributes.x = bg_state.emblem_x;
                        attributes.y = bg_state.emblem_y;
                        attributes.width = bg_state.emblem_width;
                        attributes.height = bg_state.emblem_height;

                        attributes.window_type = GDK_WINDOW_TEMP;
                        attributes.wclass = GDK_INPUT_OUTPUT;
                        attributes.visual = gdk_visual_get_system ();
                        attributes.colormap = gdk_colormap_get_system ();
                        attributes.event_mask = 0;
                        
                        window = gdk_window_new (get_root_gdk_window (), &attributes,
                                                 GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP);
                        pixmap = gdk_pixmap_new (window, bg_state.emblem_width, bg_state.emblem_height, -1);
                        background_render (&bg_state, pixmap, FALSE,
                                           bg_state.emblem_x, bg_state.emblem_y, bg_state.emblem_width, bg_state.emblem_height);

                        gdk_window_set_back_pixmap (window, pixmap, FALSE);
                        g_object_unref (pixmap);
                        
                        gdk_window_show (window);
                        gdk_window_lower (window);
                }

                /* Wait forever */
                gtk_main ();
        }

        if (run_mode == RUN_MODE_TEST) {
                GtkWidget *window;
                GdkPixmap *pixmap;

                int old_width = bg_state.width;
                int old_height = bg_state.height;

                bg_state.width /= 2;
                bg_state.height /= 2;

                window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
                gtk_widget_set_app_paintable (window, TRUE);
                gtk_widget_set_size_request (window, bg_state.width, bg_state.height);
                gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
                g_signal_connect (window, "destroy",
                                  G_CALLBACK (gtk_main_quit), NULL);

                gtk_widget_realize (window);

                pixmap = gdk_pixmap_new (window->window, bg_state.width, bg_state.height, -1);

                bg_state.emblem_x = (bg_state.emblem_x * bg_state.width) / old_width;
                bg_state.emblem_y = (bg_state.emblem_y * bg_state.height) / old_height;
                bg_state.emblem_width = (bg_state.emblem_width * bg_state.width) / old_width;
                bg_state.emblem_height = (bg_state.emblem_height * bg_state.height) / old_height;
                bg_state.tile_width = (bg_state.tile_width * bg_state.width) / old_width;
                bg_state.tile_height = (bg_state.tile_height * bg_state.height) / old_height;

                background_render (&bg_state, pixmap, FALSE, 0, 0, bg_state.width, bg_state.height);
                
                gdk_window_set_back_pixmap (window->window, pixmap, FALSE);
                g_object_unref (pixmap);

                gtk_widget_show (window);
                gtk_main ();
        }
  
        return 0;
}
