
#include <stdlib.h>
#include <glade/glade.h>
#include <time.h>
#include <string.h>

#include "app.h"

#include <gnomebt-chooser.h>


static gint conn_port=0;
static gchar *bdaddr=NULL;

static gchar *
get_current_btname (MyApp *app)
{
	gchar *name = NULL;
	if (bdaddr) {
		name = gnomebt_controller_get_device_preferred_name (
				app->btctl, bdaddr);
	}
	if (!name)
		name = g_strdup (_("No Bluetooth device chosen."));

	return name;
}

static void
set_dependent_widget (MyApp *app, gint conn_type, gboolean active)
{
	GtkWidget *w = NULL;
	switch (conn_type) {
		case CONNECTION_BLUETOOTH:
			/* only set sensitive if bluetooth available */
			if (app->btctl == NULL)
				active = FALSE;
			w = GTK_WIDGET (glade_xml_get_widget (app->ui, "btchoose"));
			gtk_widget_set_sensitive (w, active);
			break;
		case CONNECTION_OTHER:
			w = GTK_WIDGET (glade_xml_get_widget (app->ui, "otherportentry"));
			gtk_widget_set_sensitive (w, active);
			break;
	}
}

static void
on_conn_port_change (GtkWidget *widget, MyApp *app)
{
	gboolean active = gtk_toggle_button_get_active (
				GTK_TOGGLE_BUTTON (widget));
	gint port = GPOINTER_TO_INT (
			g_object_get_data (G_OBJECT (widget), "port"));

	set_dependent_widget (app, port, active);
	if (active)
		conn_port = port;
}

static gboolean
message_dialog_close (MyApp *app)
{
	GtkWidget *dialog = GTK_WIDGET (
			glade_xml_get_widget (app->ui, "sms_dialog"));
	app->showing_message = FALSE;
	gtk_widget_hide (dialog);
	return TRUE;
}

static gboolean
send_dialog_close (MyApp *app)
{
	GtkWidget *dialog = GTK_WIDGET (
			glade_xml_get_widget (app->ui, "send_dialog"));
	gtk_widget_hide (dialog);
	gtk_widget_set_sensitive (GTK_WIDGET (app->send_item), TRUE);
	return TRUE;
}

static void
set_btdevname (MyApp *app)
{
	GtkWidget *w = GTK_WIDGET (glade_xml_get_widget (app->ui, "btdevname"));
	gchar *c = get_current_btname (app);
	gtk_label_set_text (GTK_LABEL (w), c);
	g_free (c);
}

#define S_CONNECT(x, y) 	\
	w = glade_xml_get_widget (app->ui, (x)); \
	g_object_set_data (G_OBJECT (w), "port", GINT_TO_POINTER (y)); \
	g_signal_connect (G_OBJECT (w), "toggled", \
			G_CALLBACK (on_conn_port_change), (gpointer)app)
					
#define S_ACTIVE(x, y) \
	w = glade_xml_get_widget (app->ui, (x)); \
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), ctype==y);

static void
populate_prefs (MyApp *app)
{
	GtkWidget *w;
	gchar *c;
	gint ctype = gconf_client_get_int (app->client,
			CONFBASE"/connection_type", NULL);

	conn_port = ctype;

	set_dependent_widget (app, CONNECTION_BLUETOOTH,
			ctype == CONNECTION_BLUETOOTH);
	set_dependent_widget (app, CONNECTION_OTHER,
			ctype == CONNECTION_OTHER);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "otherportentry"));
	c  = gconf_client_get_string (app->client,
				CONFBASE"/other_serial", NULL);
	if (c != NULL) {
		gtk_entry_set_text (GTK_ENTRY (w), c);
		g_free (c);
	} else {
		gtk_entry_set_text (GTK_ENTRY (w), "");
	}

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "auto_retry"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
			gconf_client_get_bool (app->client,
				CONFBASE"/auto_retry", NULL));

	/* TODO: if Bluetooth isn't available, disable the bluetooth radio */

	S_ACTIVE("btdevice",  CONNECTION_BLUETOOTH);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "btdevice"));
	if (app->btctl == NULL)
		gtk_widget_set_sensitive (w, FALSE);

	S_ACTIVE("serialport1", CONNECTION_SERIAL1);
	S_ACTIVE("serialport2", CONNECTION_SERIAL2);
	S_ACTIVE("irdaport", CONNECTION_IRCOMM);
	S_ACTIVE("otherport", CONNECTION_OTHER);

	if (bdaddr)
		g_free (bdaddr);

	bdaddr = gconf_client_get_string (app->client,
			CONFBASE"/bluetooth_addr", NULL);
	set_btdevname (app);


}

static void
apply_prefs (MyApp *app, gpointer data)
{
	GtkWidget *w;

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "otherportentry"));
	gconf_client_set_string (app->client,
			CONFBASE"/other_serial",
			gtk_entry_get_text (GTK_ENTRY (w)), NULL);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "auto_retry"));
	gconf_client_set_bool (app->client,
			CONFBASE"/auto_retry",
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)), NULL);

	gconf_client_set_int (app->client,
			CONFBASE"/connection_type",
			conn_port, NULL);

	if (bdaddr != NULL)
		gconf_client_set_string (app->client,
				CONFBASE"/bluetooth_addr", bdaddr, NULL);

	reconnect_phone (app);
}

static void
choose_bdaddr (MyApp *app, gpointer data)
{
    gchar *ret = NULL;
    GnomebtController *btctl;
    GnomebtChooser *btchooser;
    gint result;

    btctl = app->btctl;
    btchooser = gnomebt_chooser_new (btctl);
    result = gtk_dialog_run (GTK_DIALOG (btchooser));

    if (result == GTK_RESPONSE_OK)
        ret = gnomebt_chooser_get_bdaddr (btchooser);

    gtk_widget_destroy (GTK_WIDGET (btchooser));

    /* consume events, so dialog gets removed from screen */
    while (gtk_events_pending ())
        gtk_main_iteration ();

	if (ret) {
		if (bdaddr)
			g_free (bdaddr);
		bdaddr = ret;
		set_btdevname (app);
	}
}

void
ui_init (MyApp *app)
{
	gchar *fname;
	GtkWidget *w;

	fname = gnome_program_locate_file (app->program,
				GNOME_FILE_DOMAIN_APP_DATADIR,
				"phonemgr.glade", TRUE, NULL);
	if (fname == NULL)
		fname = g_strdup ("../ui/phonemgr.glade");
	app->ui = glade_xml_new (fname, NULL, NULL);
	g_free (fname);

	if (!app->ui)
		g_error ("Couldn't load user interface.");

	/* close button and windowframe close button just hide the
	   prefs panel */
	
	g_signal_connect_swapped (
			G_OBJECT (glade_xml_get_widget (app->ui, "prefs_window")),
			"delete-event", G_CALLBACK (gtk_widget_hide),
			G_OBJECT (glade_xml_get_widget (app->ui, "prefs_window")));

	g_signal_connect_swapped (
			G_OBJECT (glade_xml_get_widget (app->ui, "prefsclosebutton")),
			"clicked", G_CALLBACK (gtk_widget_hide),
			G_OBJECT (glade_xml_get_widget (app->ui, "prefs_window")));

	/* prefs apply */

	g_signal_connect_swapped (
			G_OBJECT (glade_xml_get_widget (app->ui, "prefsapplybutton")),
			"clicked", G_CALLBACK (apply_prefs), (gpointer) app);

	/* bt device chooser */

	g_signal_connect_swapped (
			G_OBJECT (glade_xml_get_widget (app->ui, "btchoose")),
			"clicked", G_CALLBACK (choose_bdaddr), (gpointer) app);

	/* connect signal handlers for radio buttons */

	S_CONNECT("btdevice",  CONNECTION_BLUETOOTH);
	S_CONNECT("serialport1", CONNECTION_SERIAL1);
	S_CONNECT("serialport2", CONNECTION_SERIAL2);
	S_CONNECT("irdaport", CONNECTION_IRCOMM);
	S_CONNECT("otherport", CONNECTION_OTHER);

	app->tooltip = gtk_tooltips_new ();

	/* sms dialog stuff. every quarter second, check if there's a
	   message */
	g_timeout_add (250, (GSourceFunc) dequeue_message, (gpointer) app);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "sms_dialog"));
	g_signal_connect_swapped (G_OBJECT (w), "delete-event",
			G_CALLBACK (message_dialog_close), (gpointer) app);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "okbutton"));
	g_signal_connect_swapped(G_OBJECT (w), "clicked",
			G_CALLBACK (message_dialog_close), (gpointer) app);

	/* send dialog stuff */
	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "send_dialog"));
	g_signal_connect_swapped (G_OBJECT (w), "delete-event",
			G_CALLBACK (send_dialog_close), (gpointer) app);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "msgcancelbutton"));
	g_signal_connect_swapped (G_OBJECT (w), "clicked",
			G_CALLBACK (send_dialog_close), (gpointer) app);

}

void
show_prefs_window (MyApp *app)
{
	GtkWidget *prefs = glade_xml_get_widget (app->ui, "prefs_window");
	populate_prefs (app);
	gtk_widget_show (prefs);
}

static size_t
my_strftime(char *s, size_t max, const char  *fmt,  const
           struct tm *tm)
{
	return strftime(s, max, fmt, tm);
}

gboolean
dequeue_message (MyApp *app)
{
	GList *ptr = NULL;
	Message *msg = NULL;
	GtkDialog *dialog;
	GtkTextView *textview;
	GtkLabel *l_sender, *l_sent;
	GtkTextBuffer *buf;
	gchar work[64];
	struct tm *time_tm;

	if (app->showing_message)
		return TRUE;

	g_mutex_lock (app->message_mutex);
	ptr = g_list_first (app->messages);
	if (ptr) {
		app->messages = g_list_remove_link (app->messages, ptr);
	}
	g_mutex_unlock (app->message_mutex);
	if (!ptr)
		return TRUE;

	g_message ("Message arrived.");

	/* time to get on with displaying it */
	msg = (Message *)ptr->data;

	dialog = GTK_DIALOG (glade_xml_get_widget (app->ui, "sms_dialog"));
	l_sender = GTK_LABEL (glade_xml_get_widget (app->ui, "senderlabel"));
	l_sent = GTK_LABEL (glade_xml_get_widget (app->ui, "datelabel"));
	textview = GTK_TEXT_VIEW (glade_xml_get_widget (app->ui, "messagecontents"));
	buf = gtk_text_view_get_buffer (textview);
	gtk_text_buffer_set_text (buf, msg->message, strlen (msg->message));
	gtk_label_set_text (l_sender, msg->sender);

	time_tm = localtime ((time_t*)&msg->timestamp);
	my_strftime (work, 64, "%X %x", time_tm);
	gtk_label_set_text (l_sent, work);

	gtk_widget_show_all (GTK_WIDGET (dialog));
	app->showing_message = TRUE;

	g_free (msg->sender);
	g_free (msg->message);
	g_free (msg);
	g_list_free_1 (ptr);
	
	return TRUE;
}
