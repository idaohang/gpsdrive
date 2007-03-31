/***********************************************************************

Copyright (c) 2001,2002 Fritz Ganter <ganter@ganter.at>

Website: www.kraftvoll.at/software

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

#ifndef GPSDRIVE_TRACK_H
#define GPSDRIVE_TRACK_H

/*
 * See track.c for details.
 */

#include <gtk/gtk.h>



typedef struct
{
  gdouble lon;
  gdouble lat;
  gdouble alt;
  gchar postime[30];
}
trackcoordstruct;

void savetrackfile (gint testname);
void rebuildtracklist (void);
void drawtracks (void);
gint gettrackfile (GtkWidget *widget, gpointer datum);


#endif /* GPSDRIVE_TRACK_H */
