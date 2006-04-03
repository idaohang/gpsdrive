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

    *********************************************************************

$Log$
Revision 1.7  2006/04/03 23:43:45  tweety
rename adj --> scaler_adj
rearrange code for some of the _cb
 streets_draw_cb
 poi_draw_cb
move map_dir_struct definition to src/gpsdrive.h
remove some of the history parts in the Files
save and read settings for display_map like "display_map_<name> = 1"
increase limit for displayed streets
change color of de.Strassen.Allgemein to x555555
OSM.pm make non way segments to Strassen.Allgemein
WDB check if yountryname is valid

Revision 1.6  2006/03/10 08:37:09  tweety
- Replace Street/Track find algorithmus in Query Funktion
  against real Distance Algorithm (distance_line_point).
- Query only reports Track/poi/Streets if currently displaying
  on map is selected for these
- replace old top/map Selection by a MapServer based selection
- Draw White map if no Mapserver is selected
- Remove some useless Street Data from Examples
- Take the real colors defined in Database to draw Streets
- Add a frame to the Streets to make them look nicer
- Added Highlight Option for Tracks/Streets to see which streets are
  displayed for a Query output
- displaymap_top und displaymap_map removed and replaced by a
  Mapserver centric approach.
- Treaked a little bit with Font Sizes
- Added a very simple clipping to the lat of the draw_grid
  Either the draw_drid or the projection routines still have a slight
  problem if acting on negative values
- draw_grid with XOR: This way you can see it much better.
- move the default map dir to ~/.gpsdrive/maps
- new enum map_projections to be able to easily add more projections
  later
- remove history from gpsmisc.c
- try to reduce compiler warnings
- search maps also in ./data/maps/ for debugging purpose
- cleanup and expand unit_test.c a little bit
- add some more rules to the Makefiles so more files get into the
  tar.gz
- DB_Examples.pm test also for ../data and data directory to
  read files from
- geoinfo.pl: limit visibility of Simple POI data to a zoom level of 1-20000
- geoinfo.pl NGA.pm: Output Bounding Box for read Data
- gpsfetchmap.pl:
  - adapt zoom levels for landsat maps
  - correct eniro File Download. Not working yet, but gets closer
  - add/correct some of the Help Text
- Update makefiles with a more recent automake Version
- update po files

Revision 1.5  2006/02/05 16:38:06  tweety
reading floats with scanf looks at the locale LANG=
so if you have a locale de_DE set reading way.txt results in clearing the
digits after the '.'
For now I set the LC_NUMERIC always to en_US, since there we have . defined for numbers

Revision 1.4  2006/01/03 14:24:10  tweety
eliminate compiler Warnings
try to change all occurences of longi -->lon, lati-->lat, ...i
use  drawicon(posxdest,posydest,"w-lan.open") instead of using a seperate variable
rename drawgrid --> do_draw_grid
give the display frames usefull names frame_lat, ...
change handling of WP-types to lowercase
change order for directories reading icons
always read inconfile

Revision 1.3  1994/06/08 08:37:47  tweety
fix some ocurences of +- handling with coordinates by using coordinate_string2gdouble
instead of atof and strtod

Revision 1.2  2005/11/09 16:29:55  tweety
changed pixelfac for top_GPSWORLD
added comment to gpsd_nmea.sh

Revision 1.1  2005/11/06 17:24:26  tweety
shortened map selection code
coordinate_string2gdouble:
 - fixed missing format
 - changed interface to return gdouble
change -D option to reflect debuglevels
Added more debug Statements for Level>50
move map handling to to seperate file
speedup memory reservation for map-structure
Add code for automatic loading of maps from system DATA/maps/.. Directory
changed length of mappath from 400 to 2048 chars


 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gpsdrive.h>
#include <config.h>
#include <math.h>
#include "gpsdrive.h"
#include "speech_out.h"
#include "speech_strings.h"

/* variables */
extern gint ignorechecksum, mydebug, debug;
extern gdouble lat2RadiusArray[201];
extern gdouble zero_lon, zero_lat, target_lon, target_lat, dist;
extern gint real_screen_x, real_screen_y;
extern gint real_psize, real_smallmenu, int_padding;
extern gint SCREEN_X_2, SCREEN_Y_2;
extern gdouble pixelfact, posx, posy, angle_to_destination;
extern gdouble direction, bearing;
extern gint havepos, haveposcount, blink, gblink, xoff, yoff, crosstoogle;
extern gdouble current_lon, current_lat, old_lon, old_lat;
extern gdouble trip_lat,trip_lon;
extern gdouble groundspeed, milesconv;
extern gint nrmaps;
extern gint maploaded;
extern gint importactive;
extern gint zoom;
extern gdouble current_lon, current_lat;
extern gint debug, mydebug;
extern gint pdamode;
extern gint usesql;
extern glong mapscale;
extern gchar mapdir[500];
extern gint posmode;
extern gdouble posmode_x, posmode_y;
extern gchar targetname[40];
extern gint iszoomed;
extern gchar newmaplat[100], newmaplon[100], newmapsc[100], oldangle[100];
extern gint needtosave;

extern gint showroute, routeitems;
extern gdouble routenearest;
extern gint forcenextroutepoint;
extern gint routemode;
wpstruct *routelist;
extern gint thisrouteline, routeitems, routepointer;
extern gint gcount, milesflag, downloadwindowactive;
extern GtkWidget *drawing_area, *drawing_bearing;
extern GtkWidget *drawing_sats, *drawing_miniimage;

extern gchar oldfilename[2048];

extern gint scaleprefered, scalewanted;
extern gint borderlimit;

gchar mapfilename[2048];
extern gchar activewpfile[200];
extern gint saytarget;

extern int havedefaultmap;

extern GtkWidget *destframe;
extern gint createroute;
extern GdkPixbuf *image, *tempimage, *miniimage;

#include "gettext.h"

/*  Defines for gettext I18n */
# include <libintl.h>
# define _(String) gettext(String)
# ifdef gettext_noop
#  define N_(String) gettext_noop(String)
# else
#  define N_(String) (String)
# endif


mapsstruct *maps = NULL;
gint message_wrong_maps_shown = FALSE;
time_t maptxtstamp = 0;
gint needreloadmapconfig = FALSE;
int havenasa = -1;

GtkWidget *maptogglebt,	*topotogglebt;


gint max_display_map=0;
map_dir_struct *display_map;
gint displaymap_top = TRUE;
gint displaymap_map = TRUE;

//enum map_projections { proj_undef, proj_top, proj_map };
enum map_projections map_proj;

/* ******************************************************************
 * Find the maptype for a given Filename
 */
enum map_projections
map_projection(char *filename)
{
    enum map_projections proj = proj_undef;

    if (      ! strncmp ( filename, "expedia/",   8 ) )
	proj = proj_map;
    else if ( ! strncmp ( filename, "landsat/",   8 ) )
	proj = proj_map;
    else if ( ! strncmp ( filename, "geoscience/",11 ) )
	proj = proj_map;
    else if ( ! strncmp ( filename, "incrementp/",11 ) )
	proj = proj_map;
    else if ( ! strncmp ( filename, "gov_au/",    7 ) )
	proj = proj_map;
    else if ( ! strncmp ( filename, "_map/",      5 ) )
	proj = proj_map;
    else if ( ! strncmp ( filename, "map_",       4 ) ) // For Compatibility
	proj = proj_map;
    else if ( ! strncmp ( filename, "googlesat/", 10 ) )
	proj = proj_top;
    else if ( ! strncmp ( filename, "NASAMAPS/",  9 ) )
	proj = proj_top;
    else if ( ! strncmp ( filename, "eniro/",     6 ) )
	proj = proj_top;
    else if ( ! strncmp ( filename, "_top/",      5 ) )
	proj = proj_top;
    else if ( ! strncmp ( filename, "top_",      4 ) ) // For Compatibility
	proj = proj_top;
    else
	{
	    proj = proj_undef;
	}
    return proj;
}


/* *****************************************************************************
 */
gint
maptoggle_cb (GtkWidget * widget, guint datum)
{
	displaymap_map = !displaymap_map;
	if (displaymap_map)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (maptogglebt),
					      TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (maptogglebt),
					      FALSE);
	needtosave = TRUE;
	return TRUE;
}

/* *****************************************************************************
 */
gint
topotoggle_cb (GtkWidget * widget, guint datum)
{
	displaymap_top = !displaymap_top;
	if (displaymap_top)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (topotogglebt), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (topotogglebt), FALSE);
	needtosave = TRUE;
	return TRUE;
}

/* *****************************************************************************
 */
gint
display_maps_cb (GtkWidget * widget, guint datum)
{
    if ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (display_map[datum].checkbox)) )
    	display_map[datum].to_be_displayed = TRUE;
    else 
    	display_map[datum].to_be_displayed = FALSE;

    int i;
    for ( i = 0; i < max_display_map; i++)
	{
	    char tbd = display_map[i].to_be_displayed ? 'D' :'_';
	    printf("Found %s,%c\n",display_map[i].name,tbd);
	}

    needtosave = TRUE;
    return TRUE;
}

/* ******************************************************************
 */
GtkWidget *
make_display_map_checkboxes(){
    GtkWidget *frame_maptype;
    GtkWidget *vbox3;

    // Checkbox ---- Show Map
    frame_maptype = gtk_frame_new (_("Shown map type"));
    vbox3 = gtk_vbox_new (TRUE, 1 * PADDING);
    gtk_container_add (GTK_CONTAINER (frame_maptype), vbox3);
    
    if (0){
    // Checkbox ---- Show Map: map_
    maptogglebt = gtk_check_button_new_with_label (_("Street map"));
    if (displaymap_map)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (maptogglebt),
				      TRUE);
    gtk_signal_connect (GTK_OBJECT (maptogglebt),
			"clicked", GTK_SIGNAL_FUNC (maptoggle_cb),
			(gpointer) 1);
    gtk_box_pack_start (GTK_BOX (vbox3), maptogglebt, FALSE, FALSE,
			0 * PADDING);

    // Checkbox ---- Show Map: top_
    topotogglebt = gtk_check_button_new_with_label (_("Topo map"));
    if (displaymap_top)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (topotogglebt), TRUE);

    gtk_signal_connect (GTK_OBJECT (topotogglebt),
			"clicked", GTK_SIGNAL_FUNC (topotoggle_cb),
			(gpointer) 1);

    gtk_box_pack_start (GTK_BOX (vbox3), topotogglebt, FALSE, FALSE,
			0 * PADDING);

    }

    gint i;
    for ( i = 0; i < max_display_map; i++)
	{
	    // Checkbox ---- Show Map: name xy
	    gchar display_name[100];
	    
	    
	    if (mydebug > 1)
		g_snprintf (display_name, sizeof (display_name),"%s (%d)",
			    _( display_map[i].name),
			    display_map[i].count);
	    else
		g_snprintf (display_name, sizeof (display_name),"%s",
			    _( display_map[i].name));


	    display_map[i].count ++;

	    display_map[i].checkbox 
		= gtk_check_button_new_with_label (display_name);

	    if (display_map[i].to_be_displayed)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (display_map[i].checkbox), TRUE);
	    
	    gtk_signal_connect (GTK_OBJECT (display_map[i].checkbox),
				"clicked", GTK_SIGNAL_FUNC (display_maps_cb),
				(gpointer) i);
	    
	    gtk_box_pack_start (GTK_BOX (vbox3), display_map[i].checkbox, FALSE, FALSE,
				0 * PADDING);
	}
    
    return frame_maptype;
}

/* ******************************************************************
 * extract the directory part of the File and then 
 * add it to the  display_map[] structure
 * returns the index of the display_map[] structure
 */
int
add_map_dir(gchar *filename) {

    gint i;

    /* memorize map dir names */
    if (mydebug > 99)
	fprintf (stderr, "add_map_dir(%s)\n",filename);

    gchar map_dir[200];
    
    g_strlcpy (map_dir, filename, sizeof(map_dir));
    char *slash_pos = strstr(map_dir,"/");
    if ( slash_pos )
	slash_pos[0]='\0';
    else 
	g_strlcpy (map_dir, "no_dir", sizeof(map_dir));

    for ( i = 0; i < max_display_map; i++)
	{
	    if ( ! strcmp(display_map[i].name,map_dir ) ){
		display_map[i].count ++;
		//printf("Found %s,%s\n",display_map[i].name,map_dir);
		return i;
	    }
	}
    if (i >= max_display_map)
	{
	    max_display_map += 1;
	    display_map = g_renew(map_dir_struct, display_map, max_display_map);
	}
    g_strlcpy (display_map[i].name, map_dir, sizeof (display_map[i].name));
    display_map[i].to_be_displayed = TRUE;
    display_map[i].count ++;
    return i;
}

/* *****************************************************************************
 * map_koord.txt is in mappath! 
 */
void
test_loaded_map_names ()
{
  gint i;
  for (i = 0; i < nrmaps; i++)
  {
     if ( proj_undef == map_projection((maps +i )->filename) )
     {
	GString *error;
	error = g_string_new (NULL);
        g_string_printf (error, "%s%d\n%s\n",
		     _("Error in line "), i + 1,
		     _
		     ("I have found filenames in map_koord.txt which are\n"
		      "not map_* or top_* files. Please rename them and change the entries in\n"
		      "map_koord.txt.  Use map_* for street maps and top_* for topographical\n"
		      "maps.  Otherwise, the maps will not be displayed!"));
	error_popup ((gpointer *) error->str);
	g_string_free (error, TRUE);
	message_wrong_maps_shown = TRUE;
      }
    }
}

/* ******************************************************************
 * Check if map_koord.txt needs to be reloaded an do if so
 */
void
map_koord_check_and_reload ()
{
  gchar mappath[2048];
  struct stat buf;

  if (mydebug > 50)
    fprintf (stderr, "map_koord_check_and_reload()\n");

  /* Check for changed map_koord.txt and reload if changed */
  g_strlcpy (mappath, mapdir, sizeof (mappath));
  g_strlcat (mappath, "map_koord.txt", sizeof (mappath));

  stat (mappath, &buf);
  if (buf.st_mtime != maptxtstamp)
    {
      needreloadmapconfig = TRUE;
    }

  if (needreloadmapconfig)
    {
	loadmapconfig ();
	g_print ("%s reloaded\n", "map_koord.txt");
	maptxtstamp = buf.st_mtime;
    };
}



/* *****************************************************************************
 *  write the definitions of the map files 
 * Attention! program  writes decimal point as set in locale
 * i.eg 4.678 is in Germany 4,678 !!! 
 */
void
savemapconfig ()
{
  gchar mappath[2048];
  FILE *st;
  gint i;

  if (mapdir[strlen (mapdir) - 1] != '/')
    g_strlcat (mapdir, "/", sizeof (mapdir));

  g_strlcpy (mappath, mapdir, sizeof (mappath));
  g_strlcat (mappath, "map_koord.txt", sizeof (mappath));
  st = fopen (mappath, "w");
  if (st == NULL)
    {
      perror (mappath);
      exit (2);
    }

  for (i = 0; i < nrmaps; i++)
    {
      fprintf (st, "%s %.5f %.5f %ld\n", (maps + i)->filename,
	       (maps + i)->lat, (maps + i)->lon, (maps + i)->scale);
    }

  fclose (st);
}

/* *****************************************************************************
 *  load the definitions of the map files 
 */
gint
loadmapconfig ()
{
  gchar mappath[2048];
  FILE *st;
  gint i;
  gint max_nrmaps = 1000;
  gchar buf[1512], s1[40], s2[40], s3[40], filename[100];
  gint p, e;

  if (mydebug > 50)
    fprintf (stderr, "loadmapconfig()\n");

  init_nasa_mapfile ();
  if (mapdir[strlen (mapdir) - 1] != '/')
    g_strlcat (mapdir, "/", sizeof (mapdir));

  g_strlcpy (mappath, mapdir, sizeof (mappath));
  g_strlcat (mappath, "map_koord.txt", sizeof (mappath));
  st = fopen (mappath, "r");
  if (st == NULL)
    {
      mkdir (homedir, 0777);
      st = fopen (mappath, "w+");
      if (st == NULL)
	{
	  perror (mappath);
	  return FALSE;
	}
      st = freopen (mappath, "r", st);
      if (st == NULL)
	{
	  perror (mappath);
	  return FALSE;
	}

    }
  if (nrmaps > 0)
    g_free (maps);

  // Here I should reserve a little bit more than 1 map
  maps = g_new (mapsstruct, max_nrmaps);
  i = nrmaps = 0;
  havenasa = -1;
  while ((p = fgets (buf, 1512, st) != 0))
    {
      e = sscanf (buf, "%s %s %s %s", filename, s1, s2, s3);
      if ((mydebug > 50) && !(nrmaps % 1000))
	{
	  fprintf (stderr, "loadmapconfig(%d)\r", nrmaps);
	}


      if (e == 4)
	{
	  g_strdelimit (s1, ",", '.');
	  g_strdelimit (s2, ",", '.');
	  g_strdelimit (s3, ",", '.');

	  g_strlcpy ((maps + i)->filename, filename, 200);
	  (maps + i)->map_dir = add_map_dir(filename);
	  coordinate_string2gdouble(s1, &((maps + i)->lat));
	  coordinate_string2gdouble(s2, &((maps + i)->lon));
	  (maps + i)->scale = strtol (s3, NULL, 0);
	  i++;
	  nrmaps = i;
	  havenasa = -1;
	  if (nrmaps >= max_nrmaps)
	    {
	      max_nrmaps += 1000;
	      maps = g_renew (mapsstruct, maps, max_nrmaps);
	    }

	}
      else
	{
	  fprintf (stderr, "Line not recognized\n");
	}
    }
  fclose (st);

  needreloadmapconfig = FALSE;
  return FALSE;
}

/* *****************************************************************************
 */
void
route_next_target ()
{
  gchar str[100], buf[200], mappath[2048];
  gdouble d;
  /*  test for new route point */
  if (strcmp (targetname, "     "))
    {
      if (routemode)
	d = calcdist ((routelist + routepointer)->lon,
		      (routelist + routepointer)->lat);
      else
	d = calcdist (target_lon, target_lat);

      if (d < routenearest)
	{
	  routenearest = d;
	}
      /*    g_print */
      /*      ("\nroutepointer: %d d: %.1f routenearest: %.1f routereach: %0.3f", */
      /*       routepointer, d, routenearest, ROUTEREACH); */
      if ((d <= ROUTEREACH)
	  || (d > (ROUTEREACHFACT * routenearest)) || forcenextroutepoint)
	{
	  forcenextroutepoint = FALSE;
	  if ((routepointer != (routeitems - 1)) && (routemode))
	    {
	      routenearest = 9999999999.0;
	      routepointer++;

	      /* let's say the waypoint description */
	      g_strlcpy (mappath, homedir, sizeof (mappath));
	      g_strlcat (mappath, activewpfile, sizeof (mappath));
	      saytargettext (mappath, targetname);

	      setroutetarget (NULL, -1);
	    }
	  else
	    {
	      /*  route endpoint reached */
	      if (saytarget)
		{
		  g_snprintf (str, sizeof (str),
			      "%s: %s", _("To"), targetname);
		  gtk_frame_set_label (GTK_FRAME (destframe), str);
		  createroute = FALSE;
		  routemode = FALSE;
		  saytarget = FALSE;
		  routepointer = routeitems = 0;

		  g_snprintf (buf, sizeof (buf),
			      speech_target_reached[voicelang], targetname);
		  speech_out_speek (buf);

		  /* let's say the waypoint description */
		  g_strlcpy (mappath, homedir, sizeof (mappath));
		  g_strlcat (mappath, activewpfile, sizeof (mappath));
		  saytargettext (mappath, targetname);
		}
	    }
	}
    }
}


/* ******************************************************************
 *
 */
int
create_nasa ()
{
  gint i, j;
  gchar nasaname[255];
  int nasaisvalid = FALSE;

  if ((havenasa < 0) || (!nasaisvalid))
    {
      /* delete nasamaps entries from maps list */
      for (i = 0; i < nrmaps; i++)
	{
	  if ((strcmp ((maps + i)->filename, "top_NASA_IMAGE.ppm")) == 0)
	    {
	      for (j = i; j < (nrmaps - 1); j++)
		*(maps + j) = *(maps + j + 1);
	      nrmaps--;
	      continue;
	    }
	}

      /* Try creating a nasamap and add it to the map list */
      havenasa =
	create_nasa_mapfile (current_lat, current_lon, TRUE, nasaname);
      if (havenasa > 0)
	{
	  i = nrmaps;
	  nrmaps++;
	  maps = g_renew (mapsstruct, maps, (nrmaps + 2));
	  havenasa =
	    create_nasa_mapfile (current_lat, current_lon, FALSE, nasaname);
	  (maps + i)->lat = current_lat;
	  (maps + i)->lon = current_lon;
	  (maps + i)->scale = havenasa;
	  g_strlcpy ((maps + i)->filename, nasaname, 200);
	  if ((strcmp (oldfilename, "top_NASA_IMAGE.ppm")) == 0)
	    g_strlcpy (oldfilename, "XXXOLDMAPXXX.ppm", sizeof (oldfilename));
	}
    }
  return nasaisvalid;
}

/* ******************************************************************
 * load the map with number bestmap
 */

void
load_best_map (long long bestmap)
{
  if (bestmap != 9999999999LL)
    {
      g_strlcpy (mapfilename, (maps + bestmap)->filename,
		 sizeof (mapfilename));
      if ((strcmp (oldfilename, mapfilename)) != 0)
	{
	  g_strlcpy (oldfilename, mapfilename, sizeof (oldfilename));
	  if (debug)
	    g_print ("New map: %s\n", mapfilename);
	  pixelfact = (maps + bestmap)->scale / PIXELFACT;
	  zero_lon = (maps + bestmap)->lon;
	  zero_lat = (maps + bestmap)->lat;
	  mapscale = (maps + bestmap)->scale;
	  xoff = yoff = 0;
	  if (nrmaps > 0)
	    loadmap (mapfilename);
	}
    }
  else
    {				// No apropriate map found take worldmap
      if (((strcmp (oldfilename, mapfilename)) != 0) && (havedefaultmap))
	{
	  g_strlcpy (oldfilename, mapfilename, sizeof (oldfilename));
	  g_strlcpy (mapfilename, "top_GPSWORLD.jpg", sizeof (mapfilename));
	  pixelfact = 88067900.43 / PIXELFACT;
	  zero_lon = 0;
	  zero_lat = 0;
	  mapscale = 88067900.43;
	  xoff = yoff = 0;
	  loadmap (mapfilename);
	}
    }

}

/* *****************************************************************************
 *  We load the map 
 */
void
loadmap (char *filename)
{
    gchar mappath[2048];
    GdkPixbuf *limage;
    guchar *lpixels, *pixels;
    int i, j, k;

    if (mydebug > 10)
	fprintf (stderr, "loadmap(%s)\n", filename);

    if (maploaded)
	gdk_pixbuf_unref (image);

    map_proj = map_projection(filename);

    limage = gdk_pixbuf_new_from_file (filename, NULL);
    if (limage == NULL)
	{
	    g_snprintf (mappath, sizeof (mappath), "data/maps/%s", filename);
	    limage = gdk_pixbuf_new_from_file (mappath, NULL);
	}
    if (limage == NULL)
	{
	    g_snprintf (mappath, sizeof (mappath), "%s%s",  mapdir, filename);
	    limage = gdk_pixbuf_new_from_file (mappath, NULL);
	}
    if (limage == NULL)
	{
	    g_snprintf (mappath, sizeof (mappath), "%s/gpsdrive/maps/%s", DATADIR, filename);
	    limage = gdk_pixbuf_new_from_file (mappath, NULL);
	}

    if (limage == NULL)
	havedefaultmap = FALSE;

    if (limage == NULL)
	{
	    GString *error;
	    error = g_string_new (NULL);
	    g_string_sprintf (error, "%s\n%s\n",
			      _(" Mapfile could not be loaded:"), filename);
	    error_popup ((gpointer *) error->str);
	    g_string_free (error, TRUE);
	    maploaded = FALSE;
	    return;
	}


    if (!gdk_pixbuf_get_has_alpha (limage))
	image = limage;
    else
	{
	    image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, 1280, 1024);
	    if (image == NULL)
	{
	  fprintf (stderr,
		   "can't get image  gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, 1280, 1024)\n");
	  exit (1);
	}
      lpixels = gdk_pixbuf_get_pixels (limage);
      pixels = gdk_pixbuf_get_pixels (image);
      if (pixels == NULL)
	{
	  fprintf (stderr,
		   "can't get pixels pixels = gdk_pixbuf_get_pixels (image);\n");
	  exit (1);
	}
      j = k = 0;
      for (i = 0; i < (1280 * 1024); i++)
	{
	  memcpy ((pixels + j), (lpixels + k), 3);
	  j += 3;
	  k += 4;
	}
      gdk_pixbuf_unref (limage);

    }

  expose_cb (NULL, NULL);
  iszoomed = FALSE;
  /*        zoom = 1; */
  xoff = yoff = 0;

  rebuildtracklist ();

  if (!maploaded)
    display_status (_("Map found!"));

  maploaded = TRUE;

  /*  draw minimap */
  if (miniimage)
    gdk_pixbuf_unref (miniimage);

  miniimage = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 0, 8, 128, 103);

  gdk_pixbuf_scale (image, miniimage, 0, 0, 128, 103,
		    0, 0, 0.1, 0.10, GDK_INTERP_TILES);
  expose_mini_cb (NULL, 0);
}



/* ******************************************************************
 * test if we need to load another map 
 */
void
testnewmap ()
{
  long long best = 1000000000LL;
  gdouble posx, posy;
  long long bestmap = 9999999999LL;
  gdouble pixelfactloc, bestscale = 1000000000.0, fact;
  gint i, ncount = 0;

  //    gchar str[100], buf[200];
  gdouble dif;
  static int nasaisvalid = FALSE;

  if (importactive)
    return;

  if (posmode)
    {
	trip_lat = current_lon = posmode_x;
	trip_lon = current_lat = posmode_y;
    }
  else
    route_next_target ();


  { // Test if we want White Background as Map
      gint show_white_background=TRUE;
      
      for ( i = 0; i < max_display_map; i++)
	  {
	      if ( display_map[i].to_be_displayed ) 
		  show_white_background=FALSE;
	  }
      
      if ( show_white_background )
	  {
	      g_strlcpy (oldfilename, mapfilename, sizeof (oldfilename));
	      g_strlcpy (mapfilename, "map_White.png", sizeof (mapfilename));
	      mapscale = (glong)scalewanted;
	      pixelfact = mapscale / PIXELFACT;
	      zero_lat = current_lat;
	      zero_lon = current_lon;
	      xoff = yoff = 0;
	      map_proj = proj_map;
	      loadmap (mapfilename);
	      return;
	  }
  }


  /* search for suitable maps */
  if (displaymap_top)
    nasaisvalid = create_nasa ();
  nasaisvalid = FALSE;

  /* have a look through all the maps and decide which map 
   * is the best/apropriate
   * RESULT: bestmap [index in (maps + i) for the choosen map]
   */
  for (i = 0; i < nrmaps; i++)
    {
      /* check if map is topo or street map 
       * Result: istopo = TRUE/FALSE
       */
	
	if ( ! display_map[(maps + i)->map_dir].to_be_displayed)
	    {
		continue;
	    }

      enum map_projections proj = map_projection((maps + i)->filename);
      /*  calcxy (&posx, &posy, (maps + i)->lon, (maps + i)->lat,1); */

      /*  Longitude */
      if ( proj_map == proj )
	posx = (lat2radius ((maps + i)->lat) * M_PI / 180)
	  * cos (M_PI * (maps + i)->lat / 180.0)
	  * (current_lon - (maps + i)->lon);
      else
	posx = (lat2radius (0) * M_PI / 180)
	  * (current_lon - (maps + i)->lon);


      /*  latitude */
      if ( proj_map == proj )
	{
	  posy = (lat2radius ((maps + i)->lat) * M_PI / 180)
	    * (current_lat - (maps + i)->lat);
	  dif = lat2radius ((maps + i)->lat)
	    * (1 - (cos ((M_PI * (current_lon - (maps + i)->lon)) / 180.0)));
	  posy = posy + dif / 2.0;
	}
      else
	posy = (lat2radius (0) * M_PI / 180)
	  * (current_lat - (maps + i)->lat);


      pixelfactloc = (maps + i)->scale / PIXELFACT;
      posx = posx / pixelfactloc;
      posy = posy / pixelfactloc;

      /* */
      if (strcmp ("top_NASA_IMAGE.ppm", (maps + i)->filename) == 0)
	{
	  ncount++;
	}

      /* */
      if (((gint) posx > -(640 - borderlimit)) &&
	  ((gint) posx < (640 - borderlimit)) &&
	  ((gint) posy > -(512 - borderlimit)) &&
	  ((gint) posy < (512 - borderlimit)))
	{
	  if (displaymap_top)
	    if (strcmp ("top_NASA_IMAGE.ppm", (maps + i)->filename) == 0)
	      {
		/* nasa map is in range */
		nasaisvalid = TRUE;
	      }

	  if (scaleprefered)
	    {
	      if (scalewanted > (maps + i)->scale)
		fact = (gdouble) scalewanted / (maps + i)->scale;
	      else
		fact = (maps + i)->scale / (gdouble) scalewanted;

	      if (fact < bestscale)
		{
		  bestscale = fact;
		  bestmap = i;
		  /* bestcentereddist = centereddist; */
		}
	    }
	  else			/* autobestmap */
	    {
	      if ((maps + i)->scale < best)
		{
		  bestmap = i;
		  best = (maps + i)->scale;
		}
	    }
	}			/*  End of if posy> ... posx> ... */
    }				/* End of for ... i < nrmaps */

  // RESULT: bestmap [index in (maps + i) for the choosen map]

  load_best_map (bestmap);

}

/* *****************************************************************************
 * Robins hacking 
 * Show (in yellow) any downloaded maps with in +/-20% of the currently
 * requested map download also show bounds of map with a black border
 * This is currently hooked in to the drawdownloadrectangle() function 
 * but may be better else where as a seperate function that can be
 * turned on and off as requried.
 * Due to RGB bit masks the map to be downloaded will now be green 
 * so that the new download area will be visible over the top of the 
 * previous downloaded maps.
 */
void
drawloadedmaps ()
{
  int i;
  gdouble x, y, la, lo;
  gint scale, xo, yo;
  gchar sc[20];

  if (mydebug > 50)
    fprintf (stderr, "drawloadedmaps()\n");

  for (i = 0; i < nrmaps; i++)
    {
      g_strlcpy (sc, newmapsc, sizeof (sc));
      g_strdelimit (sc, ",", '.');
      scale = g_strtod (sc, NULL);

      if (maps[i].scale <= scale * 1.2 && maps[i].scale >= scale * 0.8)
	{
	  //printf("Selected map at lon %lf lat %lf\n",maps[i].lat,maps[i].lon);
	  la = maps[i].lat;
	  lo = maps[i].lon;
	  //              scale=maps[i].scale;
	  calcxy (&x, &y, lo, la, zoom);
	  xo = 1280.0 * zoom * scale / mapscale;
	  yo = 1024.0 * zoom * scale / mapscale;
	  // yellow background
	  gdk_gc_set_foreground (kontext, &yellow);
	  gdk_gc_set_function (kontext, GDK_AND);
	  gdk_gc_set_line_attributes (kontext, 2, 0, 0, 0);
	  gdk_draw_rectangle (drawable, kontext, 1, x - xo / 2,
			      y - yo / 2, xo, yo);
	  // solid border
	  gdk_gc_set_foreground (kontext, &black);
	  gdk_gc_set_function (kontext, GDK_SOLID);
	  gdk_gc_set_line_attributes (kontext, 2, 0, 0, 0);
	  gdk_draw_rectangle (drawable, kontext, 0, x - xo / 2,
			      y - yo / 2, xo, yo);

	}
    }
}


/* *****************************************************************************
 * draw downloadrectangle 
 */
void
drawdownloadrectangle (gint big)
{

  if (mydebug > 50)
    fprintf (stderr, "drawdownloadrectangle()\n");

  drawloadedmaps ();

  if (downloadwindowactive)
    {
      gdouble x, y, la, lo;
      gchar sc[20];
      gint scale, xo, yo;

      coordinate_string2gdouble(newmaplat, &la);
      coordinate_string2gdouble(newmaplon, &lo);

      g_strlcpy (sc, newmapsc, sizeof (sc));
      g_strdelimit (sc, ",", '.');
      scale = g_strtod (sc, NULL);

      gdk_gc_set_foreground (kontext, &green2);
      gdk_gc_set_function (kontext, GDK_AND);
      gdk_gc_set_line_attributes (kontext, 2, 0, 0, 0);
      if (big)
	{
	  calcxy (&x, &y, lo, la, zoom);
	  xo = 1280.0 * zoom * scale / mapscale;
	  yo = 1024.0 * zoom * scale / mapscale;
	  gdk_draw_rectangle (drawable, kontext, 1, x - xo / 2,
			      y - yo / 2, xo, yo);
	}
      else
	{
	  calcxymini (&x, &y, lo, la, 1);
	  xo = 128.0 * scale / mapscale;
	  yo = 102.0 * scale / mapscale;
	  gdk_draw_rectangle (drawing_miniimage->window,
			      kontext, 1, x - xo / 2, y - yo / 2, xo, yo);
	}

      gdk_gc_set_function (kontext, GDK_COPY);
    }

}

