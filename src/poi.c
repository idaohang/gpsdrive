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

*********************************************************************/


/*
 * poi_ support module: display
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>

#include <gmodule.h>
#include <gdk/gdktypes.h>
#include "gtk/gtk.h"

#include "gpsdrive.h"
#include "poi.h"
#include "config.h"
#include "gettext.h"
#include "icons.h"
#include <gpsdrive_config.h>

#include "gettext.h"

/*  Defines for gettext I18n */
# include <libintl.h>
# define _(String) gettext(String)
# ifdef gettext_noop
#  define N_(String) gettext_noop(String)
# else
#  define N_(String) (String)
# endif


extern gint do_unit_test;
extern gint maploaded;
extern gint importactive;
extern gint zoom;
extern gint showroute, routeitems;
extern gint nightmode, isnight, disableisnight;
extern GdkColor red;
extern GdkColor black;
extern GdkColor white;
extern GdkColor blue;
extern GdkColor nightcolor;
extern GdkColor mygray;
extern GdkColor textback;
extern GdkColor textbacknew;
extern GdkColor grey;

extern gdouble current_long, current_lat;
extern gint debug, mydebug;
extern GtkWidget *drawing_area, *drawing_bearing, *drawing_sats,
  *drawing_miniimage;
extern gint pdamode;
extern gint usesql;
extern glong mapscale;

char txt[5000];
PangoLayout *poi_label_layout;
#include "mysql.h"

extern MYSQL mysql;
extern MYSQL_RES *res;
extern MYSQL_ROW row;

// keep actual visible POIs in Memory
poi_struct *poi_list;
glong poi_nr;			// current number of poi to count
glong poi_max;			// max index of POIs actually in memory
glong poi_limit = -1;		// max allowed index (if you need more you have to alloc memory)
gint poi_draw = FALSE;

gchar poi_label_font[100];
GdkColor poi_colorv;
PangoFontDescription *pfd;
PangoLayout *poi_label_layout;

poi_type_struct poi_type_list[poi_type_list_max];
int poi_type_list_count = 0;

/* ******************************************************************   */

void poi_rebuild_list (void);
void get_poi_type_list (void);



/* *******************************************************
 */
void
draw_text (char *txt, gdouble posx, gdouble posy)
{
  gint width, height;
  gint k, k2;


  if (mydebug > 30)
    fprintf (stderr, "draw_text(%s,%g,%g)\n", txt, posx, posy);

  gdk_gc_set_foreground (kontext, &textback);

  poi_label_layout = gtk_widget_create_pango_layout (drawing_area, txt);
  pfd = pango_font_description_from_string ("Sans 8");
  if (poi_max > 200)
    pfd = pango_font_description_from_string ("Sans 6");

  pango_layout_set_font_description (poi_label_layout, pfd);
  pango_layout_get_pixel_size (poi_label_layout, &width, &height);
  k = width + 4;
  k2 = height;

  gdk_gc_set_function (kontext, GDK_COPY);

  gdk_gc_set_function (kontext, GDK_AND);

  {				// Draw rectangle arround Text
    // gdk_gc_set_foreground (kontext, &textbacknew);
    gdk_gc_set_foreground (kontext, &grey);
    gdk_draw_rectangle (drawable, kontext, 1,
			posx + 13, posy - k2 / 2, k + 1, k2);

  }

  poi_label_layout = gtk_widget_create_pango_layout (drawing_area, txt);
  pango_layout_set_font_description (poi_label_layout, pfd);

  gdk_draw_layout_with_colors (drawable, kontext,
			       posx + 15, posy - k2 / 2,
			       poi_label_layout, &black, NULL);
  if (poi_label_layout != NULL)
    g_object_unref (G_OBJECT (poi_label_layout));
  /* freeing PangoFontDescription, cause it has been copied by prev. call */
  pango_font_description_free (pfd);
  if (mydebug > 30)
    fprintf (stderr, " .... draw_text(%s,%g,%g)\n", txt, posx, posy);
}

/* *******************************************************
 */
void
poi_init (void)
{
  poi_limit = 40000;
  poi_list = g_new (poi_struct, poi_limit);
  if (poi_list == NULL)
    {
      g_print ("Error: Cannot allocate Memory for %ld poi\n", poi_limit);
      poi_limit = -1;
      return;
    }

  get_poi_type_list ();
  //poi_rebuild_list ();
}


/* *********************************************************
 */
gdouble poi_lat_lr = 0, poi_lon_lr = 0;
gdouble poi_lat_ul = 0, poi_lon_ul = 0;

int
poi_check_if_moved (void)
{
  gdouble lat_lr, lon_lr;
  gdouble lat_ul, lon_ul;

  if (poi_lat_lr == 0 && poi_lon_lr == 0 &&
      poi_lat_ul == 0 && poi_lon_ul == 0)
    return 1;

  calcxytopos (SCREEN_X, SCREEN_Y, &lat_lr, &lon_lr, zoom);
  calcxytopos (0, 0, &lat_ul, &lon_ul, zoom);

  if (poi_lat_lr == lat_lr && poi_lon_lr == lon_lr &&
      poi_lat_ul == lat_ul && poi_lon_ul == lon_ul)
    return 0;
  return 1;
}


/* ****************************************************************** */
/* get a list of all possible types and load their icons */
void
get_poi_type_list (void)
{
  char sql_query[3000];


  if (!usesql)
    return;

  if (mydebug > 25)
    printf ("get_poi_type_list()\n");

  {				// Delete poi_type_list
    int i;
    for (i = 0; i < poi_type_list_max; i++)
      {
	poi_type_list[i].icon = NULL;
	poi_type_list[i].name[0] = '\0';
      }
  }

  g_snprintf (sql_query, sizeof (sql_query),
	      "SELECT poi_type_id,name,scale_min,scale_max,description FROM poi_type ORDER BY poi_type_id;");

  if (mydebug > 25)
    fprintf (stderr, "get_poi_type_list: query: %s\n", sql_query);


  if (dl_mysql_query (&mysql, sql_query))
    {
      fprintf (stderr, "get_poi_type_list: Error in query: %s\n",
	       dl_mysql_error (&mysql));
      return;
    }


  if (!(res = dl_mysql_store_result (&mysql)))
    {
      fprintf (stderr, "get_poi_type_list: Error in store results: %s\n",
	       dl_mysql_error (&mysql));
      dl_mysql_free_result (res);
      res = NULL;
      return;
    }

  int counter = 0;
  while ((row = dl_mysql_fetch_row (res)))
    {
      // --------- 0: poi_type_id
      int index = (gint) g_strtod (row[0], NULL);
      if (index >= poi_type_list_max)
	{
	  fprintf (stderr,
		   "Typet_list: index(%d) > poi_type_list_max(%d)\n",
		   index, poi_type_list_max);
	  continue;
	};

      if (poi_type_list_count < index)
	poi_type_list_count = index;

      poi_type_list[index].poi_type_id = index;


      // --------- 1: symbol name
      if ( row[1] != NULL ) 
	  {
	      g_strlcpy (poi_type_list[index].name, row[1],
			 sizeof (poi_type_list[index].name));
	      g_strlcpy (poi_type_list[index].icon_name, row[1],
		     sizeof (poi_type_list[index].icon_name));

	  if (mydebug > 98)
	      printf ("get_poi_type_list: %3d:Icon '%s' for '%s'\n",
		      index, poi_type_list[index].icon_name, poi_type_list[index].name);
	  poi_type_list[index].icon =
	    read_themed_icon (poi_type_list[index].icon_name);

	  if (poi_type_list[index].icon == NULL)
	    {
	      if (mydebug > 10)
		printf ("get_poi_type_list: %3d:Icon '%s' for '%s'\tnot found\n",
			index, poi_type_list[index].icon_name, poi_type_list[index].name);
	      if (do_unit_test)
		{
		  exit (-1);
		}
	    }
	  else
	    {
	      if (mydebug > 30)
		printf
		  ("get_poi_type_list: %3d\tfor '%s'\n",
		   index, poi_type_list[index].name);
	      counter++;
	    }
	}

      // --------- 2: scale_min
      poi_type_list[index].scale_min = (gint) g_strtod (row[2], NULL);

      // --------- 3: scale_max 
      poi_type_list[index].scale_max = (gint) g_strtod (row[3], NULL);
    

      // --------- 4: description
	  g_strlcpy (poi_type_list[index].description, row[4],
	 	     sizeof (poi_type_list[index].description));
	
    }	
		

  if (!dl_mysql_eof (res))
    {
      fprintf (stderr, "get_poi_type_list: Error in dl_mysql_eof: %s\n",
	       dl_mysql_error (&mysql));
      dl_mysql_free_result (res);
      res = NULL;
      return;
    }


  dl_mysql_free_result (res);
  res = NULL;

  if (mydebug > 20)
    fprintf (stderr,
	     "get_poi_type_list: Loaded %d Icons for poi_types 0 - %d\n",
	     counter, poi_type_list_count);
}

/* *******************************************************
 * if zoom, xoff, yoff or map are changed 
 * TODO: use the real datatype for reading from database
 * (don't convert string to double)
 */
void
poi_rebuild_list (void)
{
  char sql_query[5000];
  char sql_where[5000];
  char sql_in[5000];
  struct timeval t;
  struct timeval t2;
  int r, rges;
  time_t ti;

  gdouble lat_ul, lon_ul;
  gdouble lat_ll, lon_ll;
  gdouble lat_ur, lon_ur;
  gdouble lat_lr, lon_lr;
  gdouble lat_min, lon_min;
  gdouble lat_max, lon_max;
  gdouble lat_mid, lon_mid;


  if (!usesql)
    return;

  if (!poi_draw)
    {
      if (mydebug > 20)
	printf ("poi_rebuild_list: POI_draw is off\n");
      return;
    }

  if (mydebug > 20)
    {
      printf
	("poi_rebuild_list: Start\t\t\t\t\t\tvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
    }


  if (!maploaded)
    return;

  if (importactive)
    return;


  // calculate the start and stop for lat/lon according to the displayed section
  calcxytopos (0, 0, &lat_ul, &lon_ul, zoom);
  calcxytopos (0, SCREEN_Y, &lat_ll, &lon_ll, zoom);
  calcxytopos (SCREEN_X, 0, &lat_ur, &lon_ur, zoom);
  calcxytopos (SCREEN_X, SCREEN_Y, &lat_lr, &lon_lr, zoom);

  lat_min = min (lat_ll, lat_ul);
  lat_max = max (lat_lr, lat_ur);
  lon_min = min (lon_ll, lon_ul);
  lon_max = max (lon_lr, lon_ur);

  lat_mid = (lat_min + lat_max) / 2;
  lon_mid = (lon_min + lon_max) / 2;

  gdouble poi_posx, poi_posy;
  gettimeofday (&t, NULL);
  ti = t.tv_sec + t.tv_usec / 1000000.0;

// Selection of POIs in the old way:
//
// BEGIN Code old
//
  {
  //~ {				// Limit the select with WHERE min_lat<lat<max_lat AND min_lon<lon<max_lon
    //~ g_snprintf (sql_where, sizeof (sql_where),
		//~ "WHERE ( lat BETWEEN %.6f AND %.6f ) "
		//~ "AND   ( lon BETWEEN %.6f AND %.6f ) "
		//~ // "\tAND   ( %ld  BETWEEN scale_min AND scale_max)"
		//~ " ", lat_min, lat_max, lon_min, lon_max
		//~ // , mapscale
      //~ );
    //~ g_strdelimit (sql_where, ",", '.');	// For different LANG
    //~ if (mydebug > 20)
      //~ {
	//~ printf ("poi_rebuild_list: POI mysql where: %s\n", sql_where);
	//~ printf ("poi_rebuild_list: POI mapscale: %ld\n", mapscale);
      //~ }
  //~ }

  //~ {				// Limit the displayed poi_types
    //~ g_snprintf (sql_in, sizeof (sql_in), " AND poi_type_id IN ( ");
    //~ int i;
    //~ for (i = 0; i < poi_type_list_max; i++)
      //~ {
	//~ if (poi_type_list[i].scale_min <= mapscale &&
	    //~ poi_type_list[i].scale_max >= mapscale)
	  //~ {
	    //~ gchar id_string[20];
	    //~ g_snprintf (id_string, sizeof (id_string), " %d,",
			//~ poi_type_list[i].poi_type_id);
	    //~ g_strlcat (sql_in, id_string, sizeof (sql_in));
	  //~ }
      //~ }
    //~ g_strlcat (sql_in, " 0)", sizeof (sql_in));
    //~ if (mydebug > 20)
      //~ {
	//~ printf ("POI mysql in: %s\n", sql_in);
      //~ }
  //~ }


  //~ g_snprintf (sql_query, sizeof (sql_query),
	      //~ // "SELECT lat,lon,alt,type_id,proximity "
	      //~ "SELECT lat,lon,name,poi_type_id,source_id " "FROM poi "
	      //~ //            "LEFT JOIN oi_ type ON poi_type_id = type.poi_type_id "
	      //~ "%s %s LIMIT 40000", sql_where, sql_in);
  }
  //
  // END Code old
  
  
  // this query leaves selection to sql database:
  //
  // BEGIN Code d.s.e
  // 
  {
    g_snprintf (sql_query, sizeof (sql_query),
        "SELECT poi.lat,poi.lon,poi.name,poi.poi_type_id,poi.source_id FROM poi INNER JOIN "
        "poi_type ON poi.poi_type_id=poi_type.poi_type_id "
        "WHERE ( lat BETWEEN %.6f AND %.6f ) AND ( lon BETWEEN %.6f AND %.6f ) "
        "AND ( %ld BETWEEN scale_min AND scale_max ) LIMIT 40000;",
        lat_min, lat_max, lon_min, lon_max, mapscale);
  }
  // 
  // END Code d.s.e
  
  if (mydebug > 20)
    printf ("poi_rebuild_list: POI mysql query: %s\n", sql_query);
   
  
  if (dl_mysql_query (&mysql, sql_query))
    {
      printf ("poi_rebuild_list: Error in query: \n");
      fprintf (stderr, "poi_rebuild_list: Error in query: %s\n",
	       dl_mysql_error (&mysql));
      return;
    }

  if (!(res = dl_mysql_store_result (&mysql)))
    {
      fprintf (stderr, "Error in store results: %s\n",
	       dl_mysql_error (&mysql));
      dl_mysql_free_result (res);
      res = NULL;
      return;
    }

	if (mydebug > 20)
	{
		gettimeofday(&t2, NULL);
		fprintf(stderr,"poi_rebuild_list: time for poi-selection: %d usec\n",(gint) (t2.tv_usec-t.tv_usec));
	}
	
	
  rges = r = 0;
  poi_nr = 0;
  while ((row = dl_mysql_fetch_row (res)))
    {
      rges++;
      gdouble lat, lon;


      if (mydebug > 20)
	fprintf (stderr, "Query Result: %s\t%s\t%s\t%s\n",
		 row[0], row[1], row[2], row[3]);

      lat = g_strtod (row[0], NULL);
      lon = g_strtod (row[1], NULL);
      calcxy (&poi_posx, &poi_posy, lon, lat, zoom);

      if ((poi_posx > -50) && (poi_posx < (SCREEN_X + 50)) &&
	  (poi_posy > -50) && (poi_posy < (SCREEN_Y + 50)))
	{
	  // get next free mem for point
	  poi_nr++;
	  if (poi_nr > poi_limit)
	    {
	      poi_limit = poi_nr + 10000;
	      if (mydebug > 20)
		g_print ("Try to allocate Memory for %ld poi\n", poi_limit);

	      poi_list = g_renew (poi_struct, poi_list, poi_limit);
	      if (NULL == poi_list)
		{
		  g_print ("Error: Cannot allocate Memory for %ld poi\n",
			   poi_limit);
		  poi_limit = -1;
		  return;
		}
	    }

	  // Save retrieved poi information into structure
	  (poi_list + poi_nr)->lat = lat;
	  (poi_list + poi_nr)->lon = lon;
	  (poi_list + poi_nr)->x = poi_posx;
	  (poi_list + poi_nr)->y = poi_posy;
	  g_strlcpy ((poi_list + poi_nr)->name, row[2],
		     sizeof ((poi_list + poi_nr)->name));
	  (poi_list + poi_nr)->poi_type_id = (gint) g_strtod (row[3], NULL);
	  if (mydebug > 20)
	    {
	      g_snprintf ((poi_list + poi_nr)->name,
			  sizeof ((poi_list + poi_nr)->name), "%s %s"
			  //"\n(%.4f ,%.4f)",
			  //                  (poi_list + poi_nr)->poi_type_id,
			  , row[2], row[4]
			  // , lat, lon
		);
	      /*
	       * `type_id` int(11) NOT NULL default \'0\',
	       * `alt` double default \'0\',
	       * `proximity` float default \'0\',
	       * `comment` varchar(255) default NULL,
	       * `scale_min` smallint(6) NOT NULL default \'0\',
	       * `scale_max` smallint(6) NOT NULL default \'0\',
	       * `last_modified` date NOT NULL default \'0000-00-00\',
	       * `url` varchar(160) NULL ,
	       * `address_id` int(11) default \'0\',
	       * `source_id` int(11) NOT NULL default \'0\',
	       */
	    }
	  //(poi_list + poi_nr)->wp_id      = g_strtol (row[0], NULL);
	  //      (poi_list + poi_nr)->alt        = g_strtod(row[2], NULL);
	  //(poi_list + poi_nr)->name[80] = row[4];
	  //(poi_list + poi_nr)->poi_type_id    = g_strtod(row[5], NULL); 
	  //(poi_list + poi_nr)->proximity  = g_strtod(row[6], NULL);
	  //(poi_list + poi_nr)->comment[255 ] = row[7]; 
	  //(poi_list + poi_nr)->scale_min     = row[8];  
	  //(poi_list + poi_nr)->scale_max     = row[8];  
	  //(poi_list + poi_nr)->last_modified = row[8] 
	  //(poi_list + poi_nr)->url[160]      = row[10]; 
	  //(poi_list + poi_nr)->address_id    = row[11];
	  //(poi_list + poi_nr)->source_id     = row[12]; 

	  /*
	   * if ( mydebug > 20 ) 
	   * printf ("DB: %f %f \t( x:%f, y:%f )\t%s\n",
	   * (poi_list + poi_nr)->lat, (poi_list + poi_nr)->lon, 
	   * (poi_list + poi_nr)->x, (poi_list + poi_nr)->y, 
	   * (poi_list + poi_nr)->name
	   * );
	   */
	}
    }


  poi_max = poi_nr;

  // print time for getting Data
  gettimeofday (&t, NULL);
  ti = (t.tv_sec + t.tv_usec / 1000000.0) - ti;
  if (mydebug > 20)
    printf (_("%ld(%d) rows read in %.2f seconds\n"), poi_max,
	    rges, (gdouble) ti);


  /* remember where the data belongs to */
  poi_lat_lr = lat_lr;
  poi_lon_lr = lon_lr;
  poi_lat_ul = lat_ul;
  poi_lon_ul = lon_ul;

  if (!dl_mysql_eof (res))
    {
      fprintf (stderr, "poi_rebuild_list: Error in dl_mysql_eof: %s\n",
	       dl_mysql_error (&mysql));
      dl_mysql_free_result (res);
      res = NULL;
      return;
    }

  dl_mysql_free_result (res);
  res = NULL;

  if (mydebug > 20)
    {
      printf ("poi_rebuild_list: End \t\t\t\t\t\t");
      printf ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    }
}





/* *******************************************************
 * draw poi_ on image 
 TODO: find free space on drawing area. So the Text doesn't overlap
*/
void
poi_draw_list (void)
{
  //  gint t;
  //  GdkSegment *routes;

  gint i;

  if (!usesql)
    return;

  if (importactive)
    return;

  if (!maploaded)
    return;

  if (!(poi_draw))
    {
      if (mydebug > 20)
	printf ("poi_draw_list: POI_draw is off\n");
      return;
    }


  if (mydebug > 20)
    printf
      ("poi_draw_list: Start\t\t\tvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");

  if (poi_check_if_moved ())
    poi_rebuild_list ();


  /* ------------------------------------------------------------------ */
  /*  draw poi_list points */
  if (mydebug > 20)
    printf ("poi_draw_list: drawing %ld points\n", poi_max);

  for (i = 1; i <= poi_max; i++)
    {
      gdouble posx, posy;

      posx = (poi_list + i)->x;
      posy = (poi_list + i)->y;
		
      if ((posx >= 0) && (posx < SCREEN_X) &&
	  (posy >= 0) && (posy < SCREEN_Y))
	{


/*	  
 * 	    if ( mydebug ) 
 * 	    { printf ("POI Draw: %f %f \t( x:%f, y:%f )\t%s\n",
 * 	       (poi_list + i)->lat, (poi_list + i)->lon, 
 * 	       (poi_list + i)->x, (poi_list + i)->y, 
 * 	       (poi_list + i)->name
 * 	       );
 * 		   printf("poi_nr: %ld : i: %d\n",poi_nr,i);
 *		    }
 */
	   
	  gdk_gc_set_line_attributes (kontext, 2, 0, 0, 0);

	  g_strlcpy (txt, (poi_list + i)->name, sizeof (txt));

	  //    if ((drawicon (posx, posy, "HOTEL")) == 0)
	  {
	    GdkPixbuf *icon;
	    int icon_index = (poi_list + i)->poi_type_id;
	    icon = poi_type_list[icon_index].icon;

	    if (icon != NULL && icon_index > 0)
	      {
		if (poi_max < 2000)
		  {
		    int wx = gdk_pixbuf_get_width (icon);
		    int wy = gdk_pixbuf_get_height (icon);

		    gdk_draw_pixbuf (drawable, kontext, icon,
				     0, 0,
				     posx - wx / 2,
				     posy - wy / 2,
				     wx, wy, GDK_RGB_DITHER_NONE, 0, 0);
		  }
	      }
	    else
	      {
		gdk_gc_set_foreground (kontext, &red);
		if (poi_max < 20000)
		  {		// Only draw small + if more than ... Points 
		    draw_plus_sign (posx, posy);
		  }
		else
		  {
		    draw_small_plus_sign (posx, posy);
		  }
	      }

	    // Only draw Text if less than 1000 Points are to be displayed
	    if (poi_max < 1000)
	      {
		draw_text (txt, posx, posy);
	      }
	  }
	}


    }
  if (mydebug > 20)
    printf
      ("poi_draw_list: End\t\t\t^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}


/* *******************************************************
 * query all Info for Points in area arround lat/lon
*/
void
poi_query_area (gdouble lat1, gdouble lon1, gdouble lat2, gdouble lon2)
{
  gint i;
  printf ("Query: %f ... %f , %f ... %f\n", lat1, lat2, lon1, lon2);

  for (i = 0; i < poi_max; i++)
    {
      //printf ("check POI: %ld: %f %f :%s\n",i,(poi_list + i)->lat, (poi_list + i)->lon,(poi_list + i)->name);
      if ((lat1 <= (poi_list + i)->lat) &&
	  ((poi_list + i)->lat <= lat2) &&
	  (lon1 <= (poi_list + i)->lon) && ((poi_list + i)->lon <= lon2))
	{
	  printf ("Query POI: %d: %f %f :%s\n",
		  i, (poi_list + i)->lat, (poi_list + i)->lon,
		  (poi_list + i)->name);
	}
    }
}
