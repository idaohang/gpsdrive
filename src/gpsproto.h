/* ***********************************************************************  */
/*  Prototypes */
int initkismet (void);
void get_poi_type_id_for_wlan();
int readkismet (void);
int deletesqldata (int index);
glong insertsqldata (double lat, double lon, char *name, char *typ, char *comment, int source);
gint insertsqlextradata (glong *poi_id, gchar *field_name, gchar *field_entry);
glong updatesqldata (glong poi_id, double lat, double lon, char *name, char *typ, char *comment, int source);
gint updatesqlextradata (glong *poi_id, gchar *field_name, gchar *field_entry);
glong getsqlextradata (glong *poi_id, gchar *field_name, gchar *field_entry, gchar *result);
int sqlinit (void);
void sqlend (void);
int getsqldata ();
int loadmap (char *filename);
gint zoom_cb (GtkWidget * widget, guint datum);
void splash (void);
int garblemain (int argc, char **argv);
void display_status (char *message);
gint drawmarker (GtkWidget * widget, guint * datum);
gint downloadslave_cb (GtkWidget * widget, guint datum);
gint downloadstart_cb (GtkWidget * widget, guint datum);
gint downloadsetparm (GtkWidget * widget, guint datum);
void savemapconfig ();
gint loadmapconfig ();
void loadwaypoints ();
void savewaypoints ();
void storepoint();
void speech_out_speek (char *text);
gint speech_out_init ();
gdouble calcdist (gdouble lon, gdouble lat);
gdouble calcdist2 (gdouble lon, gdouble lat);
gdouble calc_wpdist (gdouble lon1, gdouble lat1, gdouble lon2, gdouble lat2, gint from_current);
gint speech_saytime_cb (GtkWidget * widget, guint datum);
gint help_cb (GtkWidget * widget, guint datum);
gint sel_target_cb (GtkWidget * widget, guint datum);
gint import1_cb (GtkWidget * widget, guint datum);

gint friendsagent_cb (GtkWidget * widget, guint * datum);
gint addwaypoint_cb (GtkWidget * widget, gpointer datum);
void writeconfig ();
void readconfig ();
gint create_route_cb (GtkWidget * widget, guint datum);
void insertroutepoints ();
void setroutetarget ();
gint initgps ();
gint defaultserver_cb (GtkWidget * widget, guint datum);
gint expose_sats_cb (GtkWidget * widget, guint * datum);
gint testgarmin_cb (GtkWidget * widget, guint datum);
gint serialdev_cb (GtkWidget * widget, guint datum);
gint usedgps_cb (GtkWidget * widget, guint datum);
gint simfollow_cb (GtkWidget * widget, guint datum);
void saytargettext (gchar * filename, gchar * target);
void display_dsc (void);
void coordinate2gchar (gchar * text, gint buff_size, gdouble pos, gint islat, gint mode);
void checkinput (gchar * text);
void mintodecimal (gchar * text);
void mainsetup (void);
void testifnight (void);
void infos (void);
gint removesetutc (GtkWidget * widget, guint datum);
gint nav_doit (GtkWidget * widget, guint * datum);
gint expose_cb (GtkWidget * widget, guint * datum);
gint expose_compass (GtkWidget * widget, guint * datum);
gint dotripmeter (GtkWidget * widget, guint datum);
void trip (void);
gint expose_mini_cb (GtkWidget * widget, guint * datum);
void speech_out_speek_raw (char *text);
void setup_poi (void);
gint tripreset ();
int friends_sendmsg (char *serverip, char *message);
int friends_init ();
void friendssetup (void);
char *getexpediaurl ();
gint quit_program (GtkWidget * widget, gpointer datum);
gint reinitgps_cb (GtkWidget * widget, gpointer datum);
gint loadtrack_cb (GtkWidget * widget, gpointer datum);
gint about_cb (GtkWidget * widget, guint datum);
gint settings_main_cb (GtkWidget *widget, guint datum);
gint sel_message_cb (GtkWidget * widget, guint datum);
gint setmessage_cb (GtkWidget * widget, guint datum);
void signalposreq ();
gint reinsertwp_cb (GtkWidget * widget, guint datum);
GdkPixbuf *create_pixbuf (const gchar * filename);
int gpsserialinit (void);
void gpsserialquit ();
gint simulated_pos (GtkWidget * widget, guint * datum);
void speech_out_close (void);
int create_nasa_mapfile (double lat, double lon, int test, char *fn);
int init_nasa_mapfile ();
void cleanup_nasa_mapfile ();
gint checksum (gchar * text);
gint earthmate_cb (GtkWidget * widget, guint datum);
gint noserial_cb (GtkWidget * widget, guint datum);
void daylights (void);
gint setutc (GtkWidget * widget, guint datum);
gint callsqlupdateonce_cb (GtkWidget * widget, guint datum);
gint dbbuildquery_cb (GtkWidget * widget, guint datum);
G_MODULE_EXPORT gint modulesetup ();
gint vfr_cb (GtkWidget *widget, guint datum);
gint flymode_cb (GtkWidget *widget, guint datum);
gint disdev_cb (GtkWidget *widget, guint datum);
gint message_cb (char *msgid, char *name, char *text, int fs);
void exiterr (int exitcode);
void *getserialdata (void *);
int gpsserialinit (void);
void calcxy (gdouble * posx, gdouble * posy, gdouble lon, gdouble lat, gint zoom);
void minimap_xy2latlon(gint x, gint y, gdouble *lon, gdouble *lat, gdouble *dif);
void rebuildtracklist (void);
gint error_popup (gpointer datum);
void calcxymini (gdouble * posx, gdouble * posy, gdouble lon, gdouble lat,
	    gint zoom);
gdouble calcR (gdouble lat);
void calcxytopos (int , int , gdouble *, gdouble *, int );
gint navi_cb (GtkWidget * widget, guint datum);
GtkWidget* create_pixmap(GtkWidget *widget, const gchar *filename);
gint speech_out_cb (GtkWidget * widget, guint * datum);
gchar *escape_sql_string (const gchar *data);

