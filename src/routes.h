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

#ifndef GPSDRIVE_ROUTE_H
#define GPSDRIVE_ROUTE_H

#include <gtk/gtk.h>

extern gint thisrouteline;
extern GtkWidget *create_route_button;
extern GtkWidget *create_route2_button;
extern GtkWidget *select_route_button;
extern GtkWidget *gotobt;

void init_route_list ();
void free_route_list ();

void add_quickpoint_to_route ();
void add_arbitrary_point_to_route
	(gchar *name, gchar *cmt, gchar *type, gdouble lat, gdouble lon);
void route_init (gchar *name, gchar *desc, gchar *src);

void add_poi_to_route (GtkTreeModel *model, GtkTreeIter iter);

void draw_route ();
void route_settarget (gint rt_ptr);
void update_route (void);
void route_display_targetinfo (void);

enum {
	ROUTE_ID,
	ROUTE_NUMBER,
	ROUTE_ICON,
	ROUTE_NAME,
	ROUTE_DISTANCE,
	ROUTE_TRIP,
	ROUTE_LON,
	ROUTE_LAT,
	ROUTE_CMT,
	ROUTE_TYPE,
	ROUTE_COLUMS
};

#endif /* GPSDRIVE_ROUTE_H */
