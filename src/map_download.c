/***********************************************************************

Copyright (c) 2008 Guenther Meyer <d.s.e (at) sordidmusic.com>
Copyright (c) 2012 Hamish Bowman <hamish_b (at) yahoo com>

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


/*  Include Dateien */
#include "config.h"
#include <stdlib.h>
#include <string.h>
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
#include <glib/gstdio.h>

#include "gettext.h"
#include "os_specific.h"

#include <curl/curl.h>


/*  Defines for gettext I18n */
# include <libintl.h>
# define _(String) gettext(String)
# ifdef gettext_noop
#  define N_(String) gettext_noop(String)
# else
#  define N_(String) (String)
# endif


#include "gpsdrive.h"
#include "map_handler.h"
#include "gpsdrive_config.h"
#include "gui.h"
#include "map_download.h"


extern gint mydebug;
extern mapsstruct *maps;
extern gint nrmaps;
extern coordinate_struct coords;
extern currentstatus_struct current;
extern guistatus_struct gui_status;

enum
{
#ifdef serving_tiled_requests_only
	MAPSOURCE_WMS_LANDSAT,
#endif
	MAPSOURCE_TWMS_LANDSAT,
	MAPSOURCE_TMS_OSM_MAPNIK,
	MAPSOURCE_TMS_OSM_CYCLE,
	MAPSOURCE_TMS_OSM_TRANSPORT,
	MAPSOURCE_TMS_MAPQST,
	MAPSOURCE_TMS_MAPQST_AERIAL,
#ifdef server_kaput
	MAPSOURCE_TAH_OSM,
	MAPSOURCE_TAH_OSM_CYCLE,
#endif
	MAPSOURCE_N_ITEMS
};

gboolean mapdl_active = FALSE;
guint mapdl_scale;
gdouble mapdl_lat, mapdl_lon;
gchar mapdl_proj[4];  /* "top" or "map", check for override in map_projection().c */

static gchar mapdl_url[2000];
static gboolean mapdl_abort = FALSE;
static gint mapdl_zoom;
static gint server_type = -1;    /* 1=WMS Server  2=TMS Server  3=TAH server */
static gchar mapdl_file_w_path[1024];
static GtkWidget *scale_combobox, *lat_entry, *lon_entry;
static GtkWidget *mapdl_progress;
static GtkTreeModel *types_list, *scales_list;
static CURL *curl_handle;

static struct mapsource_struct
{
	gint type;        /* 1=OnEarth  2=OSM_T@H  3=OSM_cycle */
	gchar scale[50];  /* text for map scale, or name of ID */
	gint zoom;        /* WMS=0  TMS=tile zoom level */
	gint scale_int;   /* 1:nn,nnn map scale */
} mapsource[] =
{
#ifdef serving_tiled_requests_only
/* - LANDSAT data is 30m resolution per pixel, * scale factor of PIXELFACT = 1:85000,
 *     so anything finer than that is just downloading interpolated noise and you
 *     might as well just use the magnifying glass tool.
 * - At the other end, top_gpsworld covering entire planet is about 1:88 million
 * - For "map_" images anything wider than ~ 1:125k to 500k exposes the projection's
 *   distortion at wide longitudes from the center, and so a switch should be made
 *   to "top_" Plate Carr�e maps.
 */
	MAPSOURCE_WMS_LANDSAT, "NASA's OnEarth Landsat Global Mosaic", -1, -1,
	MAPSOURCE_WMS_LANDSAT, "1 : 50 000", 0, 50000,
	MAPSOURCE_WMS_LANDSAT, "1 : 75 000", 0, 75000,
	MAPSOURCE_WMS_LANDSAT, "1 : 100 000", 0, 100000,
	MAPSOURCE_WMS_LANDSAT, "1 : 250 000", 0, 250000,
	MAPSOURCE_WMS_LANDSAT, "1 : 500 000", 0, 500000,
	  /* wider scales than 1:500k switch to "top_" Plate Carr�e
	  	projection. It would be more accurate to switch nearer to
		1:125k,	but map_ is prettier so we hold on longer than we should.
		Distortion grows the further you get from lon_0; e.g. UTM
		is only	"valid" in a 6deg wide band. Usage beyond half the
		next band is not recommended, and by the time you get to
		+/-90deg from lon_0 it completely breaks. */
/* TODO: add another field for proj or use mapdl_zoom as 0/1 for map_/top_ ??? */
	MAPSOURCE_WMS_LANDSAT, "1 : 1 million", 0, 1000000,
	MAPSOURCE_WMS_LANDSAT, "1 : 2.5 million", 0, 2500000,
	MAPSOURCE_WMS_LANDSAT, "1 : 5 million", 0, 5000000,
	MAPSOURCE_WMS_LANDSAT, "1 : 10 million", 0, 10000000,
	MAPSOURCE_WMS_LANDSAT, "1 : 50 million", 0, 50000000,
#endif


	/* OnEarth @ JPL Tiled-WMS: Global Mosaic, pan sharpened visual */
	/*
	
	 OnEarth WMS has shut down for arbitrary WMS requests now and only servers
	 a subset of prescribed requrests. They must be 512x512 pixels, and on a
	 resolution matching the top-left examples in the GetTileService request.
	 Also only JPEG is available. So we'll need to fetch 6 tiles, starting with
	
	    ..,
	    .x,
	
	 the one "x" above, where our selected mapdl_lat and mapdl_lon is snapped
	 to the top-left corner of the tile grid. We'll then have to assemble the
	 images into a mosaic and crop off the right-most 256 pixels.
	 Zoom levels to list in the pull-down menu will have to be empirically
	 determined by running some downloads at mid-latitudes and seeing what
	 turns up for the scale in map_koord.txt.

      "Jpeg tiles as used by NASA WorldWind and WorldKit.
       [...]
      While the coverage is global, real data is only present between 60S and 83N.
      Configuration file for WorldWind avalaible at "http://OnEarth.jpl.nasa.gov/OnEarth.xml".
      WorldKit sample configuration for this layer at "http://OnEarth.jpl.nasa.gov/WK/visual".
      Pansharpened Data has a native resolution of 0.5 arc-seconds per pixel (15m per pixel)."
		-- http://onearth.jpl.nasa.gov/wms.cgi?request=GetTileService
	octave:
		a= [
		-180 -166 76 90
		-180 -38 -52 90
		-180 26 -116 90
		-180 58 -148 90
		-180 74 -164 90
		-180 82 -172 90
		-180 86 -176 90
		-180 88 -178 90
		-180 89 -179 90
		-180 89.5 -179.5 90
		-180 89.75 -179.75 90
		-180 89.875 -179.875 90 ]
	%	-180 89.9375 -179.9375 90 ]
		
		degrees_per_tile = [(a(:,3) - a(:,1))  (a(:,4) - a(:,2))]
		
		   256  2^8  (same for E-W and N-S, 'projection' is basic Plate Carr�e)
		   128  2^7
		    64  2^6
		    32  2^5
		    16  2^4
		     8  2^3
		     4  2^2
		     2  2^1    mapdl_zoom is type gint, so exponent to 2^ is stored as the value
		     1  2^0
		   0.5  2^-1
		  0.25  2^-2
		 0.125  2^-3

	256,128 deg per 512x512 pixel block no good for two tiles within 90N to 90S so we throw them out.
			zoom scale is 2^(zoom-200), since value of -1 has alternate meaning.
	 */
	MAPSOURCE_TWMS_LANDSAT, "NASA's OnEarth Landsat Global Mosaic", -1, -1,
	MAPSOURCE_TWMS_LANDSAT, "1 : 75 000",      197, 75000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 150 000",     198, 150000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 300 000",     199, 300000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 600 000",     200, 600000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 1 25 million",201, 1250000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 2 5 million", 202, 2500000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 5 million",   203, 5000000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 10 million",  204, 10000000,
	MAPSOURCE_TWMS_LANDSAT, "1 : 20 million",  205, 20000000,


	MAPSOURCE_TMS_OSM_MAPNIK, "OpenStreetMap's Mapnik", -1, -1,
	/* scale varies with latitude, so this is just a rough guide
		which will only be valid for mid-lats */
	/* Octave code: for lat=0:5:75; disp( [lat (a * 2*pi * pixelfact * cos(lat * pi/180)) / (256*2^9)]); end */
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 2 500", 17, 2250,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 5 000", 16, 4500,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 10 000", 15, 9000,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 20 000", 14, 18000,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 40 000", 13, 36000,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 75 000", 12, 72000,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 150 000", 11, 144000,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 300 000", 10, 288000,
	MAPSOURCE_TMS_OSM_MAPNIK, "1 : 600 000", 9, 576000,
	/* the distortion gets too bad at scales wider than 1:500k
	    It would be more accurate to stop earlier, but we compromise */


	MAPSOURCE_TMS_OSM_CYCLE, "OpenStreetMap Cycle Map", -1, -1,
	/* scale varies with latitude, so this is just a rough guide
		which will only be valid for mid-lats */
	/* Octave code: for lat=0:5:75;   disp( [lat (a * 2*pi *pixelfact * cos(lat  * pi/180))  / (256*2^9)]); end */
	// NOTE: the Cycle Map is a big flakey at closer zooms in rural areas
	//  many "404 Not Founds" are found in the PNG output file if component tiles are missing.
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 5 000", 16, 4500,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 10 000", 15, 9000,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 20 000", 14, 18000,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 40 000", 13, 36000,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 75 000", 12, 72000,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 150 000", 11, 144000,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 300 000", 10, 288000,
	MAPSOURCE_TMS_OSM_CYCLE, "1 : 600 000", 9, 576000,
	/* the distortion gets too bad at scales wider than 1:500k
	    It would be more accurate to stop earlier, but we compromise */


	MAPSOURCE_TMS_OSM_TRANSPORT, "OpenStreetMap Transport", -1, -1,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 2 500", 17, 2250,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 5 000", 16, 4500,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 10 000", 15, 9000,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 20 000", 14, 18000,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 40 000", 13, 36000,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 75 000", 12, 72000,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 150 000", 11, 144000,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 300 000", 10, 288000,
	MAPSOURCE_TMS_OSM_TRANSPORT, "1 : 600 000", 9, 576000,

	MAPSOURCE_TMS_MAPQST, "MapQuest OSM", -1, -1,
	MAPSOURCE_TMS_MAPQST, "1 : 5 000", 16, 4500,
	MAPSOURCE_TMS_MAPQST, "1 : 10 000", 15, 9000,
	MAPSOURCE_TMS_MAPQST, "1 : 20 000", 14, 18000,
	MAPSOURCE_TMS_MAPQST, "1 : 40 000", 13, 36000,
	MAPSOURCE_TMS_MAPQST, "1 : 75 000", 12, 72000,
	MAPSOURCE_TMS_MAPQST, "1 : 150 000", 11, 144000,
	MAPSOURCE_TMS_MAPQST, "1 : 300 000", 10, 288000,
	MAPSOURCE_TMS_MAPQST, "1 : 600 000", 9, 576000,

	MAPSOURCE_TMS_MAPQST_AERIAL, "MapQuest Open Aerial", -1, -1,
	MAPSOURCE_TMS_MAPQST_AERIAL, "1 : 150 000", 11, 144000,
	MAPSOURCE_TMS_MAPQST_AERIAL, "1 : 300 000", 10, 288000,
	MAPSOURCE_TMS_MAPQST_AERIAL, "1 : 600 000", 9, 576000,
	/* the distortion gets too bad at scales wider than 1:500k
	    It would be more accurate to stop earlier, but we compromise */

#ifdef server_kaput
	MAPSOURCE_TAH_OSM, "OpenStreetMap Tiles@Home", -1, -1,
	/* scale varies with latitude, so this is just a rough guide
		which will only be valid for mid-lats */
	/* Octave code: for lat=0:5:75; disp( [lat (a * 2*pi * pixelfact * cos(lat * pi/180)) / (256*2^9)]); end */
	MAPSOURCE_TAH_OSM, "1 : 2 500", 17, 2250,
	MAPSOURCE_TAH_OSM, "1 : 5 000", 16, 4500,
	MAPSOURCE_TAH_OSM, "1 : 10 000", 15, 9000,
	MAPSOURCE_TAH_OSM, "1 : 20 000", 14, 18000,
	MAPSOURCE_TAH_OSM, "1 : 40 000", 13, 36000,
	MAPSOURCE_TAH_OSM, "1 : 75 000", 12, 72000,
	MAPSOURCE_TAH_OSM, "1 : 150 000", 11, 144000,
	MAPSOURCE_TAH_OSM, "1 : 300 000", 10, 288000,
	MAPSOURCE_TAH_OSM, "1 : 600 000", 9, 576000,
	/* the distortion gets too bad at scales wider than 1:500k
	    It would be more accurate to stop earlier, but we compromise */


	MAPSOURCE_TAH_OSM_CYCLE, "OpenStreetMap Cycle Map", -1, -1,
	/* scale varies with latitude, so this is just a rough guide
		which will only be valid for mid-lats */
	/* Octave code: for lat=0:5:75;   disp( [lat (a * 2*pi *pixelfact * cos(lat  * pi/180))  / (256*2^9)]); end */
	// NOTE: the Cycle Map is a big flakey at closer zooms in rural areas
	//  many "404 Not Founds" are found in the PNG output file if component tiles are missing.
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 5 000", 16, 4500,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 10 000", 15, 9000,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 20 000", 14, 18000,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 40 000", 13, 36000,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 75 000", 12, 72000,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 150 000", 11, 144000,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 300 000", 10, 288000,
	MAPSOURCE_TAH_OSM_CYCLE, "1 : 600 000", 9, 576000,
	/* the distortion gets too bad at scales wider than 1:500k
	    It would be more accurate to stop earlier, but we compromise */
#endif

	MAPSOURCE_N_ITEMS, "", 0, 0
};


/* *****************************************************************************
 * set coordinates in text entries (called when map is clicked)
 */
void
mapdl_set_coords (gchar *lat, gchar *lon)
{
	gtk_entry_set_text (GTK_ENTRY (lat_entry), lat);
	gtk_entry_set_text (GTK_ENTRY (lon_entry), lon);
}


/* *****************************************************************************
 * download timed-WMS and its neighbors & assemble into a single 1280x1024 tile.
 * assumes lat/lon has already been snapped to top-left corner of containing tile.
 * 512x512 tiles are assumbled like:
 *    ..,
 *    .x,
 * where "x" is the containing tile, and tiles on the right side are cut in half.
 */
int dl_tiledwms(double lat, double lon, double res)
{
    gchar tilefile[512], dl_cmd[512];

    g_snprintf(tilefile, sizeof(tilefile), "top_%g_%5.3f_%5.3f.jpg",
	       res, lat, lon);

    if (mydebug > 25) {
	g_print("TWMS tile [%.9f, %.9f]  Sub-tile extent: %g degrees\n",
		lon, lat, res);
	g_print(" system( gpstiled_wms_fetch_and_assemble --res=%g --lon=%.9f --lat=%.9f --filename=%s )\n",
		res, lon, lat, tilefile);
    }
    /* todo: something better like popen(**); some kind of error checking */
    g_snprintf(dl_cmd, sizeof(dl_cmd),
    		"gpstiled_wms_fetch_and_assemble --res=%g --lon=%.9f --lat=%.9f --filename=%s",
		res, lon, lat, tilefile);

    return system(dl_cmd);
}


/* *****************************************************************************
 * calculate the local map scale based on Web Tile zoom level and latitude
 */
double
calc_webtile_scale (double lat, int zoom)
{
    double scale;
    double a = 6378137.0; /* major radius of WGS84 ellipsoid (meters) */

    scale = (a * 2*M_PI * cos(DEG2RAD(lat)) * PIXELFACT) / (256 * pow(2,zoom));

    if (mydebug > 3)
	g_print ("Tile scale: %.2f\n", scale);

    return scale;
}

/* *****************************************************************************
 * calculate the nearest tile number based on Web Tile zoom level and lat,lon
  * then download it and its neighbors & assemble into a single 1280x1024 tile.
     .....
     .....
     ..x..
     .....
*/
int calc_and_dl_webtile(double lat, double lon, int zoom)
{
    int xtile, ytile;
    gchar tilefile[512], dl_cmd[512], datasource[32];

    xtile = lon2tms_xtile(lon, zoom);
    ytile = lat2tms_ytile(lat, zoom, FALSE);

    g_snprintf(tilefile, sizeof(tilefile), "map_%d_%d_%d.png",
	       zoom, xtile, ytile);

    switch (local_config.mapsource_type)
    {
	case MAPSOURCE_TMS_OSM_MAPNIK:
		g_strlcpy(datasource, "osm_mapnik", sizeof(datasource));
		break;
	case MAPSOURCE_TMS_OSM_CYCLE:
		g_strlcpy(datasource, "osm_cycle", sizeof(datasource));
		break;
	case MAPSOURCE_TMS_OSM_TRANSPORT:
		g_strlcpy(datasource, "osm_transport", sizeof(datasource));
		break;
	case MAPSOURCE_TMS_MAPQST:
		g_strlcpy(datasource, "mapquest_open", sizeof(datasource));
		break;
	case MAPSOURCE_TMS_MAPQST_AERIAL:
		g_strlcpy(datasource, "mapquest_aerial", sizeof(datasource));
		break;
	default:
		g_print("calc_and_dl_webtile(): unknown map source\n");
		break;
    }

    if (mydebug > 25) {
	g_print("TMS tile [%d, %d]  Zoom level [%d]\n", xtile, ytile, zoom);
	g_print(" system( gpstile_fetch_and_assemble --zoom=%d --xtile=%d --ytile=%d --datasource=%s --filename=%s )\n",
		zoom, xtile, ytile, datasource, tilefile);
    }
    /* todo: something better like popen(**); some kind of error checking */
    g_snprintf(dl_cmd, sizeof(dl_cmd),
    		"gpstile_fetch_and_assemble --zoom=%d --xtile=%d --ytile=%d --datasource=%s --filename=%s",
		zoom, xtile, ytile, datasource, tilefile);

    return system(dl_cmd);
}

/* convert longitude to TMS x-tile number */
int lon2tms_xtile(double lon, int zoom)
{
    return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, zoom)));
}

/* convert latitude to TMS y-tile number */
int lat2tms_ytile(double lat, int zoom, int use_osgeo_counting)
{
    int tile_number;

    if (!use_osgeo_counting) {
	tile_number = (int)(floor(
	    (1.0 - log(
	       tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0) )
	     / M_PI)
	     / 2.0 * pow(2.0, zoom)));
    }
    else
	tile_number = -1; /* TODO */

    return tile_number;
}


/* convert TMS x-tile number to longitude of top-left corner */
double tms_xtile2lon(int xtile, int zoom)
{
    return xtile / pow(2.0, zoom) * 360.0 - 180;
}


/* convert TMS y-tile number to latitude of top-left corner */
double tms_ytile2lat(int ytile, int zoom, int use_osgeo_counting)
{
    double num_tiles, lon;

    num_tiles = M_PI - 2.0 * M_PI * ytile / pow(2.0, zoom);

    if(!use_osgeo_counting) {
	lon = 180.0 / M_PI *
	   atan(0.5 * (exp(num_tiles) - exp(-num_tiles)));
    }
    else
	lon = -9999; /* TODO */

    return lon;
}


/* *****************************************************************************
 * callback to set paramaters for map to download
 */
gint
mapdl_setparm_cb (GtkWidget *widget, gint data)
{
	G_CONST_RETURN gchar *s;
	GtkTreeIter t_iter;

	if (!gui_status.dl_window)
		return TRUE;

	if (mydebug > 25)
	    g_print ("mapdl_setparm_cb()\n");

	s = gtk_entry_get_text (GTK_ENTRY (lat_entry));
	coordinate_string2gdouble(s, &mapdl_lat);
	if (mydebug > 3)
	    g_print ("new map lat: %s\n",s);

	s = gtk_entry_get_text (GTK_ENTRY (lon_entry));
	coordinate_string2gdouble(s, &mapdl_lon);
	if (mydebug > 3)
	    g_print ("new map lon: %s\n", s);

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (scale_combobox), &t_iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (scales_list), &t_iter,
			2, &mapdl_zoom,
			3, &mapdl_scale, -1);
/* TODO: determine map_ or top_ proj at this point so drawdownloadrectangle()
 *	 knows how big to draw the green preview box */
		if (server_type == -1)
		{
			if (mydebug > 10)
			    g_print ("*** server type not set yet.\n");
			switch (local_config.mapsource_type)
			{
#ifdef serving_tiled_requests_only
			    case MAPSOURCE_WMS_LANDSAT:
				server_type = WMS_SERVER;
				break;
#endif
			    case MAPSOURCE_TWMS_LANDSAT:
				server_type = TWMS_SERVER;
				break;
#ifdef server_kaput
			    case MAPSOURCE_TAH_OSM:
			    case MAPSOURCE_TAH_OSM_CYCLE:
				server_type = TAH_SERVER;
				break;
#endif
			    case MAPSOURCE_TMS_OSM_MAPNIK:
			    case MAPSOURCE_TMS_OSM_CYCLE:
			    case MAPSOURCE_TMS_OSM_TRANSPORT:
			    case MAPSOURCE_TMS_MAPQST:
			    case MAPSOURCE_TMS_MAPQST_AERIAL:
				server_type = TMS_SERVER;
				break;
			    default:
				server_type = -1;
				g_print("mapdl_setparm_cb(): unknown map source\n");
				break;
			}
		}

		if (server_type == TAH_SERVER)
		    mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);

		if (mydebug > 3)
			g_print ("new map scale/zoom level: %d / %d\n",
				mapdl_scale, mapdl_zoom);

		local_config.mapsource_scale =
			 gtk_combo_box_get_active (GTK_COMBO_BOX (scale_combobox));

		current.needtosave = TRUE;
	}

	return TRUE;
}


/* *****************************************************************************
 * callback to set possible scales for chosen map source
 */
static gint
mapdl_setsource_cb (GtkComboBox *combo_box, gpointer data)
{
	GtkTreeIter t_iter;

	if (mydebug > 25)
	    g_print ("mapdl_setsource_cb()\n");

	if (gtk_combo_box_get_active_iter (combo_box, &t_iter))
	{
		gtk_tree_model_get (GTK_TREE_MODEL (types_list), &t_iter,
			0, &local_config.mapsource_type, -1);
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (scales_list));
		gtk_combo_box_set_active (GTK_COMBO_BOX (scale_combobox), local_config.mapsource_scale);
		current.needtosave = TRUE;

		switch (local_config.mapsource_type)
		{
#ifdef serving_tiled_requests_only
		    case MAPSOURCE_WMS_LANDSAT:
			server_type = WMS_SERVER;
			break;
#endif
		    case MAPSOURCE_TWMS_LANDSAT:
			server_type = TWMS_SERVER;
			break;
#ifdef server_kaput
		    case MAPSOURCE_TAH_OSM:
		    case MAPSOURCE_TAH_OSM_CYCLE:
			server_type = TAH_SERVER;
			break;
#endif
		    case MAPSOURCE_TMS_OSM_MAPNIK:
		    case MAPSOURCE_TMS_OSM_CYCLE:
		    case MAPSOURCE_TMS_OSM_TRANSPORT:
		    case MAPSOURCE_TMS_MAPQST:
		    case MAPSOURCE_TMS_MAPQST_AERIAL:
			server_type = TMS_SERVER;
			break;
		    default:
			server_type = -1;
			g_print("mapdl_setsource_cb(): unknown map source\n");
			break;
		}
		if (mydebug > 3)
		    g_print ("*new map source type: %d (%s)\n", local_config.mapsource_type,
			     server_type == WMS_SERVER ? "WMS server" : "TMS server");
	}

	return TRUE;
}


#ifdef INCOMPLETE_TODO
/* *****************************************************************************
 * build the url for WMS servers
 */
/* TODO: generalize to make it easy to add other Free WMS data source URLs */
void
mapdl_geturl_wms (void)
{
	switch (local_config.mapsource_type)
	{
	    case MAPSOURCE_CUSTOM_WMS:
		// ...
		break;
	    case MAPSOURCE_WMS_LANDSAT:
	    default:
		g_strlcpy(wms_url, "http://onearth.jpl.nasa.gov/wms.cgi", sizeof(wms_url));
		g_strlcpy(wms_layers, "global_mosaic", sizeof(wms_layers));  /* may be a comma separated list */
		g_strlcpy(wms_styles, "", sizeof(wms_styles));
		g_strlcpy(wms_srs, "EPSG:4326", sizeof(wms_srs));
		g_strlcpy(wms_format, "image/jpeg", sizeof(wms_format));
		break;

	}

	mapdl_geturl_landsat(wms_url, wms_layers, wms_styles, wms_srs, wms_format,
			     t_lon1, t_lat1, t_lon2, t_lat2, MAPWIDTH, MAPHEIGHT);
}
#endif


/* *****************************************************************************
 * build the url for landsat
 */
void
mapdl_geturl_landsat (void)
{
/* TODO: generalize to make it easy to add other Free WMS data source URLs */
	gdouble t_lat1, t_lat2, t_lon1, t_lon2;
	gdouble meters_per_pixel, dist_to_edge_m, dist_to_edge_deg, bump;
	gchar wms_url[512], wms_layers[512];

	meters_per_pixel = mapdl_scale / PIXELFACT;

	/* lat */
	dist_to_edge_m = meters_per_pixel * MAPHEIGHT/2;
	dist_to_edge_deg = dist_to_edge_m / (1852.0*60);
	  /* 1852m/naut mile (arc-minute of LAT) */
	t_lat1 = mapdl_lat - dist_to_edge_deg;
	t_lat2 = mapdl_lat + dist_to_edge_deg;

	/* lon */
	dist_to_edge_m = meters_per_pixel * MAPWIDTH/2;
	if (mapdl_scale <= 500000) {
	    g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj));  /* cartesian space (UTM-like) */
	    dist_to_edge_deg = dist_to_edge_m / (1852.0*60*cos(DEG2RAD(mapdl_lat)));
	      /* 1852m/naut mile (arc-minute of LAT), lon:lat ratio = cos(lat) */
	} else {
	    g_strlcpy(mapdl_proj, "top", sizeof(mapdl_proj));  /* Plate Carr�e */
	    dist_to_edge_deg = dist_to_edge_m / (1852.0*60);
	}

	t_lon1 = mapdl_lon - dist_to_edge_deg;
	t_lon2 = mapdl_lon + dist_to_edge_deg;

	/* DEBUG
	if (mydebug > 3) {
	    printf("=> mapdl_scale=%d  mapdl_zoom=%d\n", mapdl_scale, mapdl_zoom);
	    printf("=> mapdl_lat=%f  mapdl_lon=%f\n", mapdl_lat, mapdl_lon);
	    printf("=> t_lat1=%f  t_lon1=%f\n", t_lat1, t_lon1);
	    printf("=> t_lat2=%f  t_lon2=%f\n", t_lat2, t_lon2);
	}
	*/

	/* sanitize */
	/* bump has problem if > 90N-90S spans */
	/* >,< 90 lat is valid for the server, but not useful for us */
	if ( t_lat1 < -90 || t_lat2 > 90 ) {
	    if ( t_lat1 < -90 ) {
		bump = t_lat1 +90;
		t_lat1 -= bump;
		t_lat2 -= bump;
	    }
	    if ( t_lat2 > 90 ) {
		bump = t_lat2 -90;
		t_lat1 -= bump;
		t_lat2 -= bump;
	    }
	    /* recalc lon extent based on new lat */
	    mapdl_lat = (t_lat1 + t_lat2)/2;
	    if ( strcmp(mapdl_proj, "map") == 0 )
		dist_to_edge_deg = dist_to_edge_m / (1852.0*60*cos(DEG2RAD(mapdl_lat)));
	    t_lon1 = mapdl_lon - dist_to_edge_deg;
	    t_lon2 = mapdl_lon + dist_to_edge_deg;
	}

	/* valid: -180 to 360 deg */
	if ( t_lon1 < -180 ) {
	    t_lon1 += 360;
	    t_lon2 += 360;
	}

	/* DEBUG
	if (mydebug > 3) {
	    printf("-> mapdl_scale=%d  mapdl_zoom=%d\n", mapdl_scale, mapdl_zoom);
	    printf("-> mapdl_lat=%f  mapdl_lon=%f\n", mapdl_lat, mapdl_lon);
	    printf("-> t_lat1=%f  t_lon1=%f\n", t_lat1, t_lon1);
	    printf("-> t_lat2=%f  t_lon2=%f\n", t_lat2, t_lon2);
	}
	*/

	g_strlcpy(wms_url, "http://onearth.jpl.nasa.gov/wms.cgi", sizeof(wms_url));
	g_strlcpy(wms_layers, "global_mosaic", sizeof(wms_layers));  /* may be a comma separated list */

	/* EPSG code 4326 is lat/lon WGS84. Image will strech to fit within
	  * the requested bounds, so it depends on us to choose correctly
	  * so that x_scale = y_scale */

	g_snprintf (mapdl_url, sizeof (mapdl_url),
		"%s?request=GetMap"
		"&width=%d&height=%d"
		"&layers=%s&styles="
		"&srs=EPSG:4326"
		"&format=image/jpeg"
		"&bbox=%.5f,%.5f,%.5f,%.5f",
		wms_url, MAPWIDTH, MAPHEIGHT, wms_layers,
		t_lon1, t_lat1, t_lon2, t_lat2);
}


/* *****************************************************************************
 * build the url for openstreetmap_tah
 */
/* TODO: generalize to make it easy to add other Free TMS data source URLs */
void
mapdl_geturl_osm_tah (void)
{
	g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj)); /* projection is UTM-like */
	g_snprintf (mapdl_url, sizeof (mapdl_url),
		"http://server.tah.openstreetmap.org/MapOf/"
		"?lat=%.5f&long=%.5f"
		"&z=%d&w=1280&h=1024&format=png",
		mapdl_lat, mapdl_lon, mapdl_zoom);
}

/* *****************************************************************************
 * build the url for openstreetmap_cycle maps
 */
/* TODO: generalize to make it easy to add other Free TMS data source URLs */
// StaticMap API: http://ojw.dev.openstreetmap.org/StaticMap/?mode=API
/*  (StaticMap is the more modern replacement for /MapOf/) */
// e.g.: http://ojw.dev.openstreetmap.org/StaticMap/?lat=-33.9077&lon=150.4957&z=12&layer=cycle&show=1&fmt=png&w=1280&h=1024&att=none
/* CycleMap may not be any good closer than zoom level=13 */
void
mapdl_geturl_osm_cycle (void)
{
	g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj)); /* projection is UTM-like */
	g_snprintf (mapdl_url, sizeof (mapdl_url),
		"http://ojw.dev.openstreetmap.org/StaticMap/"
		"?lat=%.5f&lon=%.5f"
		"&z=%d&w=1280&h=1024&format=png"
		"&layer=cycle&show=1&att=none",
		mapdl_lat, mapdl_lon, mapdl_zoom);
// fixme: need some sort of test to make sure it built right:
//  if [ `grep -c '404 Not Found' map_$zoom_$lat_$lon.png` -gt 0 ] ...
}

/* *****************************************************************************
 * do the actual download work
 */
void
mapdl_download (void)
{
	FILE *map_file;
	struct stat file_stat;
	int stat_ret;
	gchar tmp_dl_file_w_path[4096];
	char *basename;
	int xtile, ytile;
	double bbox_n, bbox_s, bbox_e, bbox_w, center_lat, center_lon;

	if (mydebug > 25)
	    g_print ("mapdl_download()\n");

	mapdl_active = TRUE;

	if (g_file_test (mapdl_file_w_path, G_FILE_TEST_IS_REGULAR))
	{
		if (popup_yes_no (NULL,
		   _("The map file already exists, overwrite?"))
		   == GTK_RESPONSE_NO)
		{
			mapdl_active = FALSE;
			return;
		}
	}


	switch (server_type)
	{
	    case TMS_SERVER:
		if (calc_and_dl_webtile(mapdl_lat, mapdl_lon, mapdl_zoom) != 0)
		{
		    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 0.0);
		    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
			    _("Download failed."));
		}
		else
		{
		    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 1.0);
		    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
			    _("Download complete."));
		}
		/* move patched mosiac to where it needs to go: */
		basename = g_path_get_basename(mapdl_file_w_path);
		g_snprintf(tmp_dl_file_w_path, sizeof(tmp_dl_file_w_path),
			    "%s/.tmp/%s", local_config.dir_maps, basename);
		g_free(basename);
		/* assume that all of .gpsdrive/maps/ is on the same drive */
		rename(tmp_dl_file_w_path, mapdl_file_w_path);
		break;

	    case TWMS_SERVER:
		if (dl_tiledwms(mapdl_lat, mapdl_lon, pow(2, mapdl_zoom-200)) != 0)
		{
		    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 0.0);
		    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
			    _("Download failed."));
		}
		else
		{
		    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 1.0);
		    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
			    _("Download complete."));
		}
		/* move patched mosiac to where it needs to go: */
		basename = g_path_get_basename(mapdl_file_w_path);
		g_snprintf(tmp_dl_file_w_path, sizeof(tmp_dl_file_w_path),
			    "%s/.tmp/%s", local_config.dir_maps, basename);
		g_free(basename);
		/* assume that all of .gpsdrive/maps/ is on the same drive */
		rename(tmp_dl_file_w_path, mapdl_file_w_path);
		break;

	    default:
		map_file = fopen (mapdl_file_w_path, "w");
		curl_easy_setopt (curl_handle, CURLOPT_URL, mapdl_url);
		curl_easy_setopt (curl_handle, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt (curl_handle, CURLOPT_WRITEDATA, map_file);

		set_cursor_style (CURSOR_WATCH);
	
		if (curl_easy_perform (curl_handle))
		{
		    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 0.0);
		    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
			    _("Aborted."));
		}
		else
		{
		    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 1.0);
		    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
			    _("Download complete."));
		}
		fclose (map_file);
		break;
	}

	/* add new map to map_koords.txt */
	stat_ret = g_stat (mapdl_file_w_path, &file_stat);

	if (stat_ret == 0 && file_stat.st_size > 0)
	{
		loadmapconfig ();
		maps = g_renew (mapsstruct, maps, (nrmaps + 2));

		g_strlcpy ((maps + nrmaps)->filename,
			&mapdl_file_w_path[strlen (local_config.dir_maps)], 200);

		(maps + nrmaps)->map_dir =
			add_map_dir (&mapdl_file_w_path[strlen (local_config.dir_maps)]);

		(maps + nrmaps)->hasbbox = FALSE;

		switch (server_type)
		{
		    case TMS_SERVER:
			xtile = lon2tms_xtile(mapdl_lon, mapdl_zoom);
			ytile = lat2tms_ytile(mapdl_lat, mapdl_zoom, FALSE);
			/* todo: use bounding box of assembed 5x4 tile array for new-style map_koord.txt entry */
			bbox_n = tms_ytile2lat(ytile - 2, mapdl_zoom, FALSE);
			bbox_s = tms_ytile2lat(ytile + 1 + 1, mapdl_zoom, FALSE);
			bbox_e = tms_xtile2lon(xtile + 2 + 1, mapdl_zoom);
			bbox_w = tms_xtile2lon(xtile - 2, mapdl_zoom);
			center_lat = (bbox_n + bbox_s) / 2;
			center_lon = (bbox_e + bbox_w) / 2;
			(maps + nrmaps)->lat = center_lat;
			(maps + nrmaps)->lon = center_lon;
			(maps + nrmaps)->scale = calc_webtile_scale(center_lat, mapdl_zoom);
			break;
		    case TWMS_SERVER:
			/* todo: use bounding box of assembed 3x2 tile array for new-style map_koord.txt entry */
			bbox_n = mapdl_lat + pow(2, mapdl_zoom-200);
			bbox_s = mapdl_lat - pow(2, mapdl_zoom-200);
			bbox_e = mapdl_lon + (1.5 * pow(2, mapdl_zoom-200));
			bbox_w = mapdl_lon - pow(2, mapdl_zoom-200);
			center_lat = (bbox_n + bbox_s) / 2;
			center_lon = (bbox_e + bbox_w) / 2;
			(maps + nrmaps)->lat = center_lat;
			(maps + nrmaps)->lon = center_lon;
			(maps + nrmaps)->scale = (int)((2 * pow(2, mapdl_zoom-200) * (1852.0*60) / MAPHEIGHT)
						   * PIXELFACT + 0.5);
			if (mydebug > 25)
			    g_print ("center_lat=%.9f  center_lon=%.9f  scale=%ld\n",
				     center_lat, center_lon, (maps + nrmaps)->scale);
			break;
		    default:
			(maps + nrmaps)->lat = mapdl_lat;
			(maps + nrmaps)->lon = mapdl_lon;
			(maps + nrmaps)->scale = mapdl_scale;
			break;
		}
		nrmaps++;
		savemapconfig ();
	}
	else {
		if (mydebug > 0)
		    g_print ("mapdl_download() failed\n");
		g_remove (mapdl_file_w_path);
	}

	set_cursor_style (CURSOR_DEFAULT);
	mapdl_active = FALSE;
}


/* *****************************************************************************
 * update the progress bar while downloading
 */
gint mapdl_progress_cb (gpointer clientp, gdouble dltotal, gdouble dlnow, gdouble ultotal, gdouble ulnow)
{
	gchar t_buf[24];
	if (mapdl_abort)
		return 1;

	g_snprintf (t_buf, sizeof (t_buf), "Downloaded %d kBytes", (gint) dlnow/1024);
	/* as the current servers don't send a file size in the header, we
	 * pulse the progress bar instead of filling it */
	//gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), dlnow/dltotal);
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (mapdl_progress));
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress), t_buf);
	while (gtk_events_pending ())
		gtk_main_iteration ();

	return 0;
}


/* *****************************************************************************
 * start the map download
 */
gint mapdl_start_cb (GtkWidget *widget, gpointer data)
{
	gchar file_path[512];
	gchar path[40];
	gchar img_fmt[4];
	gchar scale_str[40];

	if (mapdl_active)
		return TRUE;

	if (mydebug > 25)
	    g_print ("mapdl_start_cb()\n");

	/* preset filename and build download url */
	switch (local_config.mapsource_type)
	{
#ifdef serving_tiled_requests_only
		case MAPSOURCE_WMS_LANDSAT:
			g_strlcpy (path, "landsat", sizeof (path));
			g_strlcpy (img_fmt, "jpg", sizeof (img_fmt));
			mapdl_geturl_landsat ();
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_scale);
			break;
#endif
		case MAPSOURCE_TWMS_LANDSAT:
			g_strlcpy (path, "landsat", sizeof (path));
			g_strlcpy (img_fmt, "jpg", sizeof (img_fmt));
			g_strlcpy(mapdl_proj, "top", sizeof(mapdl_proj));
			g_snprintf (scale_str, sizeof (scale_str), "%g", pow(2, mapdl_zoom-200));
			break;
#ifdef server_kaput
		case MAPSOURCE_TAH_OSM:
			g_strlcpy (path, "openstreetmap_tah", sizeof (path));
			g_strlcpy (img_fmt, "png", sizeof (img_fmt));
			mapdl_geturl_osm_tah ();
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;
		case MAPSOURCE_TAH_OSM_CYCLE:
			g_strlcpy (path, "openstreetmap_cycle", sizeof (path));
			g_strlcpy (img_fmt, "png", sizeof (img_fmt));
			mapdl_geturl_osm_cycle ();
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;
#endif
		case MAPSOURCE_TMS_OSM_MAPNIK:
			g_strlcpy (path, "openstreetmap_tms_mapnik", sizeof (path));
			g_strlcpy (img_fmt, "png", sizeof (img_fmt));
			g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj));
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;
		case MAPSOURCE_TMS_OSM_CYCLE:
			g_strlcpy (path, "openstreetmap_tms_cycle", sizeof (path));
			g_strlcpy (img_fmt, "png", sizeof (img_fmt));
			g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj));
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);			
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;
		case MAPSOURCE_TMS_OSM_TRANSPORT:
			g_strlcpy (path, "openstreetmap_tms_xport", sizeof (path));
			g_strlcpy (img_fmt, "png", sizeof (img_fmt));
			g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj));
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;
		case MAPSOURCE_TMS_MAPQST:
			g_strlcpy (path, "openstreetmap_tms_mapqst", sizeof (path));
			g_strlcpy (img_fmt, "jpg", sizeof (img_fmt));
			g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj));
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);			
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;

		case MAPSOURCE_TMS_MAPQST_AERIAL:
			g_strlcpy (path, "openstreetmap_tms_mapqst_aerial", sizeof (path));
			g_strlcpy (img_fmt, "jpg", sizeof (img_fmt));
			g_strlcpy(mapdl_proj, "map", sizeof(mapdl_proj));
			mapdl_scale = (int)calc_webtile_scale(mapdl_lat, mapdl_zoom);
			g_snprintf (scale_str, sizeof (scale_str), "%d", mapdl_zoom);
			break;
		default:
			return TRUE;
	}

	if (mydebug > 5)
		g_print ("  download url:\n%s\n", mapdl_url);


	/* snap lat,long to upper-left corner of nearest tile grid node */
	if (server_type == TWMS_SERVER)
	{
	    /* Round lat to nearest grid line.
	     * Keep longitude at jumping to left version as we crop off the right-hand 256 pixels */
	    mapdl_lat = 90.0 - round((90.0 - mapdl_lat) / pow(2, mapdl_zoom-200)) * pow(2, mapdl_zoom-200);
	    mapdl_lon = -180.0 + floor((180.0 + mapdl_lon) / pow(2, mapdl_zoom-200)) * pow(2, mapdl_zoom-200);
	}

	/* set file path and create directory if necessary */
	g_snprintf (file_path, sizeof (file_path), "%s%s/%s/%.0f/%.0f/",
		local_config.dir_maps, path, scale_str, mapdl_lat, mapdl_lon);

	if(!g_file_test (file_path, G_FILE_TEST_IS_DIR))
	{
		if (g_mkdir_with_parents (file_path, 0700))
			g_print ("Error creating %s\n",file_path);
		else if (mydebug > 10)
			printf("created directory %s\n",file_path);
	}

	/* complete filename */
	if (server_type == TMS_SERVER)
	{
	    g_snprintf (mapdl_file_w_path, sizeof (mapdl_file_w_path), "%s%s_%s_%d_%d.%s",
		file_path, mapdl_proj, scale_str,
		lon2tms_xtile(mapdl_lon, mapdl_zoom),
		lat2tms_ytile(mapdl_lat, mapdl_zoom, FALSE), img_fmt);
	}
	else
	{
	    g_snprintf (mapdl_file_w_path, sizeof (mapdl_file_w_path), "%s%s_%s_%5.3f_%5.3f.%s",
		file_path, mapdl_proj, scale_str, mapdl_lat, mapdl_lon, img_fmt);
	}

	if (mydebug > 10)
		g_print ("  filename: %s\n", mapdl_file_w_path);

	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mapdl_progress),
		_("Loading Map..."));

	mapdl_abort = FALSE;
	mapdl_download ();

	return TRUE;
}


/* *****************************************************************************
 * set the text, that is shown in the comboboxes
 */
static gboolean
mapdl_set_combo_filter_cb (GtkTreeModel *model, GtkTreeIter *iter, gpointer scales)
{
	gint t_type, t_flag;

	if (mydebug > 30)
	    g_print ("mapdl_set_combo_cb()\n");

	if (scales)
	{
		gtk_tree_model_get (model, iter,
			0, &t_type,
			2, &t_flag, -1);
		if (t_flag != -1  && t_type == local_config.mapsource_type)
			return TRUE;
	}
	else
	{
		gtk_tree_model_get (model, iter,
			2, &t_flag, -1);
		if (t_flag == -1)
			return TRUE;
	}

	return FALSE;
}


/* *****************************************************************************
 * close map download window
 */
gint
mapdl_close_cb (GtkWidget *widget, gpointer data)
{
	if (mapdl_active)
	{
		mapdl_abort = TRUE;
		return TRUE;
	}

	gui_status.dl_window = mapdl_active = FALSE;
	gtk_widget_destroy (widget);
	expose_mini_cb (NULL, 0);
	set_cursor_style (CURSOR_DEFAULT);

	return TRUE;
}


/* *****************************************************************************
 * create map download dialog
 */
gint
map_download_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *mapdl_dialog, *mapdl_box, *mapdl_table;
	GtkWidget *dl_bt, *close_bt;
	GtkWidget *lat_lb, *lon_lb, *source_lb;
	GtkWidget *help_lb, *scale_lb;
//	GtkWidget *cover_lb, *cover_entry;
	GtkWidget *source_combobox;
	GtkListStore *source_list;
	GtkCellRenderer *renderer_source, *renderer_scale;
	GtkTreeIter t_iter;

	gchar t_buf[300];
	gint i;

	if (mydebug > 25)
	    g_print ("map_download_cb()\n");

	curl_handle = curl_easy_init();
	curl_easy_setopt (curl_handle, CURLOPT_PROGRESSFUNCTION, mapdl_progress_cb);

	set_cursor_style (CURSOR_CROSS);

	source_list = gtk_list_store_new (4,
		G_TYPE_INT,		/* type (WMS, TMS, T@H) */
		G_TYPE_STRING,		/* map scale text or name */
		G_TYPE_INT,		/* tile zoom level */
		G_TYPE_INT		/* scale_int */
		);

	for (i = 0; mapsource[i].type != MAPSOURCE_N_ITEMS; i++)
	{
		gtk_list_store_append (source_list, &t_iter);
		gtk_list_store_set (source_list, &t_iter,
			0, mapsource[i].type,
			1, mapsource[i].scale,
			2, mapsource[i].zoom,
			3, mapsource[i].scale_int,
			-1);
	}
	types_list = gtk_tree_model_filter_new (GTK_TREE_MODEL (source_list), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (types_list),
		mapdl_set_combo_filter_cb, (gpointer) 0, NULL);
	scales_list = gtk_tree_model_filter_new (GTK_TREE_MODEL (source_list), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (scales_list),
		mapdl_set_combo_filter_cb, (gpointer) 1, NULL);

	mapdl_dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (mapdl_dialog), _("Select coordinates and scale"));
	gtk_container_set_border_width (GTK_CONTAINER (mapdl_dialog), 5);
	gtk_window_set_position (GTK_WINDOW (mapdl_dialog), GTK_WIN_POS_CENTER);
	mapdl_box = gtk_vbox_new (TRUE, 2);

	dl_bt = gtk_button_new_with_label (_("Download map"));
	GTK_WIDGET_SET_FLAGS (dl_bt, GTK_CAN_DEFAULT);
	gtk_window_set_default (GTK_WINDOW (mapdl_dialog), dl_bt);
	g_signal_connect (dl_bt, "clicked", G_CALLBACK (mapdl_start_cb), 0);

	close_bt = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	g_signal_connect_swapped (close_bt, "clicked", G_CALLBACK (mapdl_close_cb), mapdl_dialog);
	g_signal_connect_swapped (mapdl_dialog, "delete_event", G_CALLBACK (mapdl_close_cb), mapdl_dialog);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mapdl_dialog)->action_area), dl_bt, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mapdl_dialog)->action_area), close_bt, TRUE, TRUE, 2);

	source_lb = gtk_label_new (_("Map Source"));
	source_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (types_list));
	gtk_combo_box_set_active (GTK_COMBO_BOX (source_combobox), local_config.mapsource_type);
	g_signal_connect (source_combobox, "changed", G_CALLBACK (mapdl_setsource_cb), NULL);
	renderer_source = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (source_combobox), renderer_source, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (source_combobox),
		renderer_source, "text", 1, NULL);

//	cover_lb = gtk_label_new (_("Map covers"));
//	cover_entry = gtk_entry_new ();
//	gtk_editable_set_editable (GTK_EDITABLE (cover_entry), FALSE);
//	g_signal_connect (cover_entry, "changed", G_CALLBACK (mapdl_setparm_cb), NULL);

	lat_lb = gtk_label_new (_("Latitude"));
	lat_entry = gtk_entry_new ();
	coordinate2gchar(t_buf, sizeof(t_buf), coords.current_lat, TRUE, local_config.coordmode);
	gtk_entry_set_text (GTK_ENTRY (lat_entry), t_buf);
	g_signal_connect (lat_entry, "changed", G_CALLBACK (mapdl_setparm_cb), NULL);

	lon_lb = gtk_label_new (_("Longitude"));
	lon_entry = gtk_entry_new ();
	coordinate2gchar(t_buf, sizeof(t_buf), coords.current_lon, FALSE, local_config.coordmode);
	gtk_entry_set_text (GTK_ENTRY (lon_entry), t_buf);
	g_signal_connect (lon_entry, "changed", G_CALLBACK (mapdl_setparm_cb), NULL);

	scale_combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (scales_list));
	g_signal_connect (scale_combobox, "changed", G_CALLBACK (mapdl_setparm_cb), NULL);
	renderer_scale = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (scale_combobox), renderer_scale, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (scale_combobox),
		renderer_scale, "text", 1, NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (scale_combobox), 0);

	g_snprintf (t_buf, sizeof (t_buf), "%s",
		_("You can also select the position\nwith a mouse click on the map."));
	help_lb = gtk_label_new (t_buf);
	scale_lb = gtk_label_new (_("Scale"));

	mapdl_progress = gtk_progress_bar_new ();
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mapdl_progress), 0.0);
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (mapdl_progress), 0.01);

	mapdl_table = gtk_table_new (7, 2, FALSE);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), source_lb, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), source_combobox, 1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), lat_lb, 0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), lat_entry, 1, 2, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), lon_lb, 0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), lon_entry, 1, 2, 2, 3);
// "Map covers" is not implemented (and I'm not sure it can be)
//	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), cover_lb, 0, 1, 3, 4);
//	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), cover_entry, 1, 2, 3, 4);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), scale_lb, 0, 1, 4, 5);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), scale_combobox, 1, 2, 4, 5);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), help_lb, 0, 2, 5, 6);
	gtk_table_attach_defaults (GTK_TABLE (mapdl_table), mapdl_progress, 0, 2, 6, 7);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (mapdl_dialog)->vbox), mapdl_table, TRUE, TRUE, 2);

	gtk_widget_show_all (mapdl_dialog);
	gui_status.dl_window = TRUE;
	mapdl_setparm_cb (NULL, 0);

	return TRUE;
}


/* *****************************************************************************
 * init curl for map download
 */
gint
mapdl_init (void)
{
	if (curl_global_init (CURL_GLOBAL_ALL))
	{
		g_print (_("Initialization of CURL for map download failed!\n"));
		return FALSE;
	}
	return TRUE;
}


/* *****************************************************************************
 * cleanup curl for map download
 */
gint
mapdl_cleanup (void)
{
	curl_global_cleanup ();
	return TRUE;
}
