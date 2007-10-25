/***********************************************************************

Copyright (c) 2007 Guenther Meyer <d.s.e (at) sordidmusic.com>

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
 * module for gpx import and export
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <gmodule.h>
#include <gdk/gdktypes.h>
#include "gtk/gtk.h"

#include "gpsdrive.h"
#include "config.h"
#include "gettext.h"
#include <gpsdrive_config.h>
#include "gpx.h"

#include "gettext.h"
#include <libxml/xmlreader.h>

/*  Defines for gettext I18n */
# include <libintl.h>
# define _(String) gettext(String)
# ifdef gettext_noop
#  define N_(String) gettext_noop(String)
# else
#  define N_(String) (String)
# endif


extern gint mydebug;
extern GtkWidget *main_window;

enum node_type
{
	NODE_START = 1,
	NODE_ATTR,
	NODE_TEXT,
	NODE_CDATA,
	NODE_ENTITY_REFERENCE,
	NODE_ENTITY_DECLARATION,
	NODE_PI,
	NODE_COMMENT,
	NODE_DOC,
	NODE_DTD,
	NODE_DOC_FRAGMENT,
	NODE_NOTATION,
	NODE_END = 15
};


typedef struct
{
 gchar version[255];
 gchar creator[255];
 gchar name[255];
 gchar desc[255];
 gchar author[255];
 gchar email[255];
 gchar url[255];
 gchar urlname[255];
 gchar time[255];
 gchar keywords[255];
 gdouble bounds[4];
 gint wpt_count;
 gint rte_count;
 gint trk_count;
} gpx_info_struct;


gpx_info_struct gpx_info;


void test_gpx (gchar *filename)
{
	gpx_file_read (filename, GPX_INFO);

	fprintf (stdout, "\nTESTING GPX-FILE: %s\n", filename);
	fprintf (stderr, "==================================================\n");
	fprintf (stderr, "Version     : %s\n", gpx_info.version);
	fprintf (stderr, "Creator     : %s\n", gpx_info.creator);
	fprintf (stderr, "Name        : %s\n", gpx_info.name);
	fprintf (stderr, "Description : %s\n", gpx_info.desc);
	fprintf (stderr, "Author      : %s\n", gpx_info.author);
	fprintf (stderr, "Email       : %s\n", gpx_info.email);
	fprintf (stderr, "URL         : %s\n", gpx_info.url);
	fprintf (stderr, "URL Name    : %s\n", gpx_info.urlname);
	fprintf (stderr, "Time        : %s\n", gpx_info.time);
	fprintf (stderr, "Keywords    : %s\n", gpx_info.keywords);
	fprintf (stderr, "Bounds      : %.6f / %.6f - %.6f / %.6f\n\n",
		gpx_info.bounds[0], gpx_info.bounds[1],
		gpx_info.bounds[2], gpx_info.bounds[3]);
	fprintf (stderr, "The file contains %d waypoints, %d routes, and %d tracks.\n",
		gpx_info.wpt_count, gpx_info.rte_count, gpx_info.trk_count);
	fprintf (stderr, "==================================================\n\n");
}


/* ******************************************************************
 * Get file info stored in gpx file.
 * Possible Nodes:
 *  version, creator, name, desc, author, email, url, urlname,
 *  time, keywords, bounds
 */
void gpx_handle_gpxinfo (xmlTextReaderPtr xml_reader, xmlChar *node_name)
{
	gint xml_status = 0;

	if (xmlStrEqual(node_name, BAD_CAST "gpx"))
	{
		g_snprintf(gpx_info.version, sizeof(gpx_info.version), "%s",
			xmlTextReaderGetAttribute(xml_reader, BAD_CAST "version"));
		g_snprintf(gpx_info.creator, sizeof(gpx_info.version), "%s",
			xmlTextReaderGetAttribute(xml_reader, BAD_CAST "creator"));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "name"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.name, sizeof(gpx_info.name),
			"%s", xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "desc"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.desc, sizeof(gpx_info.desc),
			"%s", xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "author"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.author, sizeof(gpx_info.author), "%s",
			xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "email"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.email, sizeof(gpx_info.email), "%s",
			xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "url"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.url, sizeof(gpx_info.url),
			"%s", xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "urlname"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.urlname, sizeof(gpx_info.urlname), "%s",
			xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "time"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.time, sizeof(gpx_info.time),
		"%s", xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "keywords"))
	{
		xml_status = xmlTextReaderRead(xml_reader);
		g_snprintf(gpx_info.keywords, sizeof(gpx_info.keywords),
			"%s", xmlTextReaderConstValue(xml_reader));
	}
	else if (xmlStrEqual(node_name, BAD_CAST "bounds"))
	{
		gpx_info.bounds[0] = g_strtod ((gpointer) xmlTextReaderGetAttribute(xml_reader, BAD_CAST "minlat"), NULL);
		gpx_info.bounds[1] = g_strtod ((gpointer) xmlTextReaderGetAttribute(xml_reader, BAD_CAST "minlon"), NULL);
		gpx_info.bounds[2] = g_strtod ((gpointer) xmlTextReaderGetAttribute(xml_reader, BAD_CAST "maxlat"), NULL);
		gpx_info.bounds[3] = g_strtod ((gpointer) xmlTextReaderGetAttribute(xml_reader, BAD_CAST "maxlon"), NULL);
	}
	else return;
}


void gpx_handle_wpt ()
{

}


void gpx_handle_rte ()
{

}


void gpx_handle_trk ()
{

}


/* ******************************************************************
 * Read a given gpx file, gpx_mode sets the type of data to read.
 * Possible values: GPX_INFO, GPX_WPT, GPX_RTE, GPX_TRK
 */
gint gpx_file_read (gchar *gpx_file, gint gpx_mode)
{
	xmlTextReaderPtr xml_reader;
	gint xml_status = 0; // 0 = empty, 1 = node, -1 = error
	gint node_type = 0;
	xmlChar *node_name = NULL;

	if (mydebug > 10)
		fprintf (stdout, "gpx_file_read (%s, %d)\n", gpx_file, gpx_mode);

	xml_reader = xmlNewTextReaderFilename(gpx_file);

	if (xml_reader == NULL)
	{
		fprintf (stderr, "gpx_file_read: File %s not found!\n", gpx_file);
		return FALSE;
	}

	/* reset gpx info structure */
	gpx_info.wpt_count = gpx_info.rte_count = gpx_info.trk_count = 0;
	gpx_info.bounds[0] = gpx_info.bounds[1] = gpx_info.bounds[2] = gpx_info.bounds[3] = 0;
	g_strlcpy (gpx_info.version, _("n/a"), sizeof (gpx_info.version));
	g_strlcpy (gpx_info.creator, _("n/a"), sizeof (gpx_info.creator));
	g_strlcpy (gpx_info.name, _("n/a"), sizeof (gpx_info.name));
	g_strlcpy (gpx_info.desc, _("n/a"), sizeof (gpx_info.desc));
	g_strlcpy (gpx_info.author, _("n/a"), sizeof (gpx_info.author));
	g_strlcpy (gpx_info.email, _("n/a"), sizeof (gpx_info.email));
	g_strlcpy (gpx_info.url, _("n/a"), sizeof (gpx_info.url));
	g_strlcpy (gpx_info.urlname, _("n/a"), sizeof (gpx_info.urlname));
	g_strlcpy (gpx_info.time, _("n/a"), sizeof (gpx_info.time));
	g_strlcpy (gpx_info.keywords, _("n/a"), sizeof (gpx_info.keywords));

	/* parse complete gpx file */
	xml_status = xmlTextReaderRead(xml_reader);
	while (xml_status == 1)
	{
		node_type = xmlTextReaderNodeType(xml_reader);
		node_name = xmlTextReaderName(xml_reader);
		
		if (node_type == NODE_START)
		{
			if (xmlStrEqual(node_name, BAD_CAST "wpt"))
			{
				if (gpx_mode == GPX_WPT)
					gpx_handle_wpt ();
				else
				{
					gpx_info.wpt_count++;
					while (!xmlStrEqual(node_name, BAD_CAST "wpt") || node_type != NODE_END)
					{
						xml_status = xmlTextReaderRead(xml_reader);
						node_type = xmlTextReaderNodeType(xml_reader);
						node_name = xmlTextReaderName(xml_reader);
					}
				}
			}
			else if (xmlStrEqual(node_name, BAD_CAST "rte"))
			{
				if (gpx_mode == GPX_RTE)
					gpx_handle_rte ();
				else
				{
					gpx_info.rte_count++;
					while (!xmlStrEqual(node_name, BAD_CAST "rte") || node_type != NODE_END)
					{
						xml_status = xmlTextReaderRead(xml_reader);
						node_type = xmlTextReaderNodeType(xml_reader);
						node_name = xmlTextReaderName(xml_reader);
					}
				}
			}
			else if (xmlStrEqual(node_name, BAD_CAST "trk"))
			{
				if (gpx_mode == GPX_TRK)
					gpx_handle_trk ();
				else
				{
					gpx_info.trk_count++;
					while (!xmlStrEqual(node_name, BAD_CAST "trk") || node_type != NODE_END)
					{
						xml_status = xmlTextReaderRead(xml_reader);
						node_type = xmlTextReaderNodeType(xml_reader);
						node_name = xmlTextReaderName(xml_reader);
					}
				}
			}
			else
				gpx_handle_gpxinfo (xml_reader, node_name);
		}
		xml_status = xmlTextReaderRead(xml_reader);
	}

	xmlFreeTextReader(xml_reader);
	if (node_name)
		xmlFree (node_name);
	
	if (xml_status != 0)
	{
		fprintf(stderr, "gpx_file_read: Failed to parse file: %s\n", gpx_file);
		return FALSE;
	}

	return TRUE;
}


/* ******************************************************************
 * Write data to a gpx file, gpx_mode sets the type of data to write.
 * Possible Values: GPX_WPT, GPX_RTE, GPX_TRK
 */
gint gpx_file_write (gchar *gpx_file, gint gpx_mode)
{

	return TRUE;
}


/* *****************************************************************************
 * Callback for toggling of gpx info display in the file dialog
 */
static void
toggle_file_info_cb (GtkWidget *button, GtkWidget *dialog)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		gtk_file_chooser_set_preview_widget_active
			(GTK_FILE_CHOOSER (dialog), TRUE);
	else
		gtk_file_chooser_set_preview_widget_active
		(GTK_FILE_CHOOSER (dialog), FALSE);
}


/* *****************************************************************************
 * Callback for updating the displayed gpx info in the file dialog
 */
static void
update_file_info_cb (GtkWidget *dialog, GtkWidget *label)
{
	gchar buf[2048];
	gchar *filename;

	filename = gtk_file_chooser_get_preview_filename (GTK_FILE_CHOOSER (dialog));
	if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) == FALSE)
	{
		g_snprintf (buf, sizeof (buf), _("Please select a file"));
	}
	else if (gpx_file_read (filename, GPX_INFO))
	{
		g_snprintf (buf, sizeof (buf),
			_("<b>Name:</b>\n<i>%s</i>\n<b>Description:</b>\n<i>%s</i>\n<b>Author</b>:\n<i>%s</i>\n<b>created by:</b>\n<i>%s</i>\n<b>Timestamp:</b>\n<i>%s</i>"),
			gpx_info.name, gpx_info.desc, gpx_info.author,
			gpx_info.creator, gpx_info.time);
	}
	else
	{
		g_snprintf (buf, sizeof (buf), "no valid GPX file");
	}
	gtk_label_set_markup (GTK_LABEL (label), buf);
	g_free (filename);
}


/* *****************************************************************************
 * Dialog: Load GPX File, including preview for gpx information
 */
gint loadgpx_cb (gint gpx_mode)
{
	GtkWidget *fdialog;
	GtkFileFilter *filter_gpx;
	GtkWidget *info_bt, *info_lb;

	/* create filedialog with gpx filter */
	fdialog = gtk_file_chooser_dialog_new (_("Select a GPX file"),
		GTK_WINDOW (main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_window_set_modal (GTK_WINDOW (fdialog), TRUE);
	gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (fdialog),
		local_config.dir_home, NULL);
	filter_gpx = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter_gpx, "*.gpx");
	gtk_file_filter_add_pattern (filter_gpx, "*.GPX");
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (fdialog), filter_gpx);

	/* create widget for additional file info */
	info_lb = gtk_label_new (NULL);
	gtk_label_set_width_chars (GTK_LABEL (info_lb), 30);
	gtk_label_set_line_wrap (GTK_LABEL (info_lb), TRUE);
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (fdialog), info_lb);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (fdialog), FALSE);
	gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (fdialog), FALSE);
	g_signal_connect (fdialog, "update-preview",
		G_CALLBACK (update_file_info_cb), info_lb);
	info_bt = gtk_check_button_new_with_label (_("Show Info"));
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (fdialog), info_bt);
	g_signal_connect (info_bt, "toggled",
		G_CALLBACK (toggle_file_info_cb), fdialog);

	/* preset directory depending on wanted data */
	switch (gpx_mode)
	{
		case GPX_RTE:
			gtk_file_chooser_set_current_folder
			(GTK_FILE_CHOOSER (fdialog), local_config.dir_routes);
			break;
		case GPX_TRK:
			gtk_file_chooser_set_current_folder
			(GTK_FILE_CHOOSER (fdialog), local_config.dir_tracks);
			break;
		default:
			gtk_file_chooser_set_current_folder
			(GTK_FILE_CHOOSER (fdialog), local_config.dir_home);
			break;
	}

	if (gtk_dialog_run (GTK_DIALOG (fdialog)) == GTK_RESPONSE_ACCEPT)
	{
		test_gpx (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fdialog)));
	}

	gtk_widget_destroy (fdialog);
	return TRUE;
}
