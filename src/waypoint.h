/***********************************************************************

Copyright (c) 2001,2002 Fritz Ganter <ganter@ganter.at>

Website: www.gpsdrive.de/

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

#ifndef GPSDRIVE_WAYPOINT_H
#define GPSDRIVE_WAYPOINT_H

/*
 * See waypoint.c for details.
 */

gint addwaypoint_cb (GtkWidget * widget, gpointer datum);

glong addwaypoint (gchar * wp_name, gchar * wp_type, gchar * wp_comment,
		   gdouble wp_lat, gdouble wp_lon, gboolean save_in_db);

void quickaddwaypoint (void);
void check_and_reload_way_txt();
void draw_waypoints();
void loadwaypoints();

#endif /* GPSDRIVE_WAYPOINT_H */
