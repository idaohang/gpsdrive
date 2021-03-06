/***********************************************************************

Copyright (c) 2001-2004 Fritz Ganter <ganter@ganter.at>

Website: www.gpsdrive.de

Disclaimer: Please do not use for navigation. 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


***********************************************************************/

/*  Include Dateien */
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#include <semaphore.h>
#include <gpsdrive_config.h>

#include "gettext.h"

#include "LatLong-UTMconversion.h"
#include "gpsdrive.h"
#include "battery.h"
#include "track.h"
#include "poi.h"
#include "icons.h"
#include "gui.h"
#include "main_gui.h"


/*  Defines for gettext I18n */
# include <libintl.h>
# define _(String) gettext(String)
# ifdef gettext_noop
#  define N_(String) gettext_noop(String)
# else
#  define N_(String) (String)
# endif


#if GTK_MINOR_VERSION < 2
#define gdk_draw_pixbuf _gdk_draw_pixbuf
#endif


#define	MAX_ICONS	MAXPOITYPES

extern gint do_unit_test;
extern gint debug;
extern gint mydebug;
extern color_struct colors;
extern GdkGC *kontext_map;

icons_buffer_struct icons_buffer[MAX_ICONS];
gint icons_buffer_max = MAX_ICONS;
gint icons_buffer_last = 0;

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


/* *****************************************************************************
 * draw a + Sign and its shaddow */
void
draw_plus_sign (gdouble posxdest, gdouble posydest)
{
  gdk_gc_set_line_attributes (kontext_map, 1, 0, 0, 0);
  if (local_config.showshadow)
    {				/*  draw shadow of + sign */
      gdk_gc_set_foreground (kontext_map, &colors.darkgrey);
      gdk_gc_set_function (kontext_map, GDK_AND);
      gdk_draw_line (drawable, kontext_map,
		     posxdest + 1 + SHADOWOFFSET,
		     posydest + 1 - 5 + SHADOWOFFSET, posxdest + 1 + SHADOWOFFSET, posydest + 1 + 5 + SHADOWOFFSET);
      gdk_draw_line (drawable, kontext_map,
		     posxdest + 1 + 5 + SHADOWOFFSET,
		     posydest + 1 + SHADOWOFFSET, posxdest + 1 - 5 + SHADOWOFFSET, posydest + 1 + SHADOWOFFSET);
      gdk_gc_set_function (kontext_map, GDK_COPY);
    }

  /*  draw + sign at destination */
  gdk_gc_set_foreground (kontext_map, &colors.red);
  gdk_draw_line (drawable, kontext_map, posxdest + 1, posydest + 1 - 5, posxdest + 1, posydest + 1 + 5);
  gdk_draw_line (drawable, kontext_map, posxdest + 1 + 5, posydest + 1, posxdest + 1 - 5, posydest + 1);

}

/* *****************************************************************************
 * draw a small + Sign and its shaddow */
void
draw_small_plus_sign (gdouble posxdest, gdouble posydest)
{
  gdk_gc_set_line_attributes (kontext_map, 1, 0, 0, 0);
  if (local_config.showshadow)
    {				/*  draw shadow of + sign */
      gdk_gc_set_foreground (kontext_map, &colors.darkgrey);
      gdk_gc_set_function (kontext_map, GDK_AND);
      gdk_draw_line (drawable, kontext_map,
		     posxdest + 1 + SHADOWOFFSET,
		     posydest + 1 - 2 + SHADOWOFFSET, posxdest + 1 + SHADOWOFFSET, posydest + 1 + 2 + SHADOWOFFSET);
      gdk_draw_line (drawable, kontext_map,
		     posxdest + 1 + 2 + SHADOWOFFSET,
		     posydest + 1 + SHADOWOFFSET, posxdest + 1 - 2 + SHADOWOFFSET, posydest + 1 + SHADOWOFFSET);
      gdk_gc_set_function (kontext_map, GDK_COPY);
    }

  /*  draw + sign at destination */
  gdk_gc_set_foreground (kontext_map, &colors.red);
  gdk_draw_line (drawable, kontext_map, posxdest + 1, posydest + 1 - 2, posxdest + 1, posydest + 1 + 2);
  gdk_draw_line (drawable, kontext_map, posxdest + 1 + 2, posydest + 1, posxdest + 1 - 2, posydest + 1);

}


/* -----------------------------------------------------------------------------
 * 
*/
int
drawicon (gint posxdest, gint posydest, char *icon_name)
{
  int symbol = 0, aux = -1, i;
  int wx, wy;
  gchar icon[80];

  //printf("drawicon %d %d %s\n", posxdest,  posydest,icon_name);

  g_strlcpy (icon, icon_name, sizeof (icon));

  /* sweep through all icons and look for icon */
  for (i = 0; i < icons_buffer_last; i++)
    if ((strcmp (icon, icons_buffer[i].name)) == 0)
      {
	if ((posxdest >= 0) && (posxdest < gui_status.mapview_x) && (posydest >= 0) && (posydest < gui_status.mapview_y))
	  {
	    wx = gdk_pixbuf_get_width (icons_buffer[i].icon);
	    wy = gdk_pixbuf_get_height (icons_buffer[i].icon);
	    gdk_draw_pixbuf (drawable, kontext_map,
			     (icons_buffer + i)->icon, 0, 0,
			     posxdest - wx / 2, posydest - wy / 2, wx, wy, GDK_RGB_DITHER_NONE, 0, 0);
	    aux = i;
	  }
	return 99999;
      }

  if (symbol == 0)
    {
      draw_plus_sign (posxdest, posydest);
      return 0;
    }

  return symbol;
}

typedef struct
{
  gchar *path;
  gchar *option;
} path_definition;

/* -----------------------------------------------------------------------------
 * load icon into pixbuff from either system directory or user directory
 * if force is set we exit on non success
*/
GdkPixbuf *
read_icon (gchar * icon_name, int force)
{
  gchar filename[1024];
  gchar icon_filename[1024];
  GdkPixbuf *icons_buffer = NULL;

  path_definition available_path[] = {
    {"", NULL},
    {"./data/map-icons/", NULL},
    {"../data/map-icons/", NULL},
    {"./data/pixmaps/", NULL},
    {"../data/pixmaps/", NULL},
    {"%spixmaps/", (gchar *) local_config.dir_home},
    {"%smap-icons/", (gchar *) local_config.dir_home},
    {"%s/icons/openstreetmap/", (gchar *) DATADIR},
    {"%s/icons/map-icons/", (gchar *) DATADIR},
    {"%s/map-icons/", (gchar *) DATADIR},
    {"%s/gpsdrive/pixmaps/", (gchar *) DATADIR},
    {"%s/icons/map-icons/", "/usr/share"},
    {"%s/gpsdrive/pixmaps/", "/usr/share"},
    {"END", NULL}
  };

  gint i;

  if (mydebug > 50)
    fprintf (stderr, "read_icon(%s,%d)\n", icon_name, force);
  
  for (i = 0; strncmp (available_path[i].path, "END", sizeof (available_path[i].path)); i++)
    {
      g_snprintf (filename, sizeof (filename), available_path[i].path, available_path[i].option);
      g_snprintf (icon_filename, sizeof (icon_filename), "%s%s", filename, icon_name);
      if (mydebug > 75)
	fprintf (stderr, "read_icon(%s): Try\t%s\n", icon_name, icon_filename);
      icons_buffer = gdk_pixbuf_new_from_file (icon_filename, NULL);
      if (NULL != icons_buffer)
	{
	  if (mydebug > 20)
	    fprintf (stderr, "read_icon(%s): FOUND\t%s\n", icon_name, icon_filename);
	  return icons_buffer;
	}
    }

  if (NULL == icons_buffer && force)
    {
      fprintf (stderr, "read_icon: No Icon '%s' found\n", icon_name);
      if (strstr (icon_name, "Old") == NULL)
	{
	  fprintf (stderr, _("Please install the program as root with:\n" "make install\n\n"));

	  fprintf (stderr, "I searched in :\n");
	  for (i = 0; strncmp (available_path[i].path, "END", sizeof (available_path[i].path)); i++)
	    {
	      g_snprintf (filename, sizeof (filename), available_path[i].path, available_path[i].option);
	      g_snprintf (icon_filename, sizeof (icon_filename), "%s%s", filename, icon_name);
	      fprintf (stderr, "\t%s\n", icon_filename);
	    }

	  exit (-1);
	}
    }

  return icons_buffer;
}


/* *******************************************************
 * read icon in the selected theme.
 * this includes reducing the icon path if this icon is not found 
 * until the first parent icon is found
 */

GdkPixbuf *
read_themed_icon (gchar * icon_name)
{
  gchar themed_icon_filename[2048];
  gchar icon_file_name[2048];
  GdkPixbuf *icon = NULL;
  char *p_pos;

  if (0 >= (int) strlen (icon_name))
    {
      fprintf (stderr, "read_themed_icon([%s] '%s') => Empy Icon name requested\n", local_config.icon_theme, icon_name);
      return NULL;
    }

  g_strlcpy (icon_file_name, icon_name, sizeof (icon_file_name));
  g_strdelimit (icon_file_name, ".", '/');

  do
    {
      g_snprintf (themed_icon_filename, sizeof (themed_icon_filename),
		  "%s/%s.png", local_config.icon_theme, icon_file_name);
      if (mydebug > 90)
	fprintf (stderr, "read_themed_icon([%s] %s) => Themed File %s\n",
		 local_config.icon_theme, icon_name, themed_icon_filename);
      icon = read_icon (themed_icon_filename, 0);
      if (icon != NULL)
	return icon;

      p_pos = strrchr (icon_file_name, '/');
      if (p_pos)
	{
	  p_pos[0] = '\0';
	  if (mydebug > 6)
	    fprintf (stderr, "read_themed_icon([%s] %s) reduced to => %s\n",
		     local_config.icon_theme, icon_name, icon_file_name);
	}
    }
  while (p_pos != NULL);
  if (NULL == icon)
    {
      if (mydebug > 0)
          fprintf (stderr, "read_themed_icon([%s] %s): No Icon '%s' found for theme %s\n",
	       local_config.icon_theme, icon_name, icon_name, local_config.icon_theme);
      if ( do_unit_test ) {
	  exit (-1);
      }
    }
  return NULL;
}


/* -----------------------------------------------------------------------------
*/

/* ----------------------------------------------------------------------------- */
/* warning: still modifies icon_name 
 */
void
load_user_icon (char icon_name[200])
{
  int i;
  char path[1024];
  for (i = 0; i < (int) strlen (icon_name); i++)
    icon_name[i] = tolower (icon_name[i]);

  g_snprintf (path, sizeof (path), "%sicons/%s.png", local_config.dir_home, icon_name);
  icons_buffer[icons_buffer_last].icon = gdk_pixbuf_new_from_file (path, NULL);

  if (icons_buffer[icons_buffer_last].icon == NULL)
    {
      g_snprintf (path, sizeof (path), "%s/map-icons/%s.png", DATADIR, icon_name);
      icons_buffer[icons_buffer_last].icon = gdk_pixbuf_new_from_file (path, NULL);
    }

  if ((icons_buffer + icons_buffer_last)->icon != NULL)
    {
      for (i = 0; i < (int) strlen (icon_name); i++)
	icon_name[i] = tolower (icon_name[i]);
      if ((strcmp (icon_name, "wlan") == 0) || (strcmp (icon_name, "wlan-wep") == 0))
	{
	  fprintf (stderr, _("Loaded user defined icon %s\n"), path);
	}
      else
	{
	  g_strlcpy ((icons_buffer + icons_buffer_last)->name, icon_name, sizeof (icons_buffer->name));
	  fprintf (stderr, _("Loaded user defined icon %s\n"), path);
	  icons_buffer_last++;
	}
      if (mydebug > 3)
	fprintf (stderr, "Icon for %s loaded:%s\n", icon_name, path);
    }
  else
    {
      if (mydebug > 3)
	fprintf (stderr, "No Icon for %s loaded\n", icon_name);
    }
}


/* *****************************************************************************
 * draw wlan Waypoints
 */
void
drawwlan (gint posxdest, gint posydest, gint wlan)
{
  /*  wlan=0: no wlan, 1:open wlan, 2:WEP crypted wlan */

  if (wlan == 0)
    return;

  if ((posxdest >= 0) && (posxdest < gui_status.mapview_x))
    {
      if ((posydest >= 0) && (posydest < gui_status.mapview_y))
	{
	  if (wlan == 1)
	    drawicon (posxdest, posydest, "wlan.open");
	  else
	    drawicon (posxdest, posydest, "wlan.wep");
	}
    }
}
