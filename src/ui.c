
#include <glib/gi18n.h>
#include <stdlib.h>
#include <glade/glade.h>
#include <time.h>
#include <string.h>

#include "app.h"
#include "e-phone-entry.h"
#include "phonemgr-utils.h"

#include <gnomebt-chooser.h>

#define MAX_MESSAGE_LENGTH 160

static int conn_port=0;
static char *bdaddr=NULL;

static void
boldify_label (GladeXML *xml, const char *name)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget (xml, name);

	if (widget == NULL) {
		/* g_warning ("widget '%s' not found", name); */
		return;
	}

	/* this way is probably better, but for some reason doesn't work with
	 * labels with mnemonics.

	static PangoAttrList *pattrlist = NULL;

	if (pattrlist == NULL) {
		PangoAttribute *attr;

		pattrlist = pango_attr_list_new ();
		attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
		attr->start_index = 0;
		attr->end_index = G_MAXINT;
		pango_attr_list_insert (pattrlist, attr);
	}
	gtk_label_set_attributes (GTK_LABEL (widget), pattrlist);*/

	char *str_final;
	str_final = g_strdup_printf ("<b>%s</b>", gtk_label_get_label (GTK_LABEL (widget)));
	gtk_label_set_markup_with_mnemonic (GTK_LABEL (widget), str_final);
	g_free (str_final);
}

static void
bigger_label (GladeXML *xml, const char *name)
{
	GtkWidget *widget;

	widget = glade_xml_get_widget (xml, name);

	if (widget == NULL) {
		/* g_warning ("widget '%s' not found", name); */
		return;
	}

	char *str_final;
	str_final = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", gtk_label_get_label (GTK_LABEL (widget)));
	gtk_label_set_markup_with_mnemonic (GTK_LABEL (widget), str_final);
	g_free (str_final);
}

static char *
get_resource (MyApp *app, char *uiresname)
{
	char *fname;

	fname = g_build_filename ("..", "ui", uiresname, NULL);

	if (g_file_test (fname, G_FILE_TEST_EXISTS) == FALSE) {
		g_free (fname);
		fname = g_build_filename (DATA_DIR, uiresname, NULL);
	}

	if (g_file_test (fname, G_FILE_TEST_EXISTS) == FALSE) {
		g_free (fname);
		fname = NULL;
	}

	return fname;
}

static
GladeXML *get_ui (MyApp *app, char *widget)
{
	char *fname;
	GladeXML *ui;

	fname = get_resource (app, "phonemgr.glade");

	ui = glade_xml_new (fname, widget, NULL);
	g_free (fname);

	boldify_label (ui, "alerting_label");
	boldify_label (ui, "phone_connection_label");
	boldify_label (ui, "error_handling_label");
	bigger_label (ui, "new_message_label");
	bigger_label (ui, "enter_message_label");

	return ui;
}

static void
play_sound (MyApp *app, const char *filename)
{
	GstElement *audio_sink;
	char *uri;

	/* Setup the playbin */
	if (app->playbin == NULL) {
		audio_sink = gst_element_factory_make ("gconfaudiosink", "audio-sink");
		if (audio_sink == NULL)
			audio_sink = gst_element_factory_make ("autoaudiosink", "audio-sink");
		if (g_object_class_find_property (G_OBJECT_GET_CLASS (audio_sink), "profile"))
			g_object_set (G_OBJECT (audio_sink), "profile", 0, NULL);
		app->playbin = gst_element_factory_make ("playbin", "play");
		g_object_set (app->playbin, "audio-sink", audio_sink, NULL);
	}

	/* Play the file */
	uri = g_filename_to_uri (filename, NULL, NULL);
	gst_element_set_state (app->playbin, GST_STATE_NULL);
	g_object_set (G_OBJECT (app->playbin), "uri", uri, NULL);
	gst_element_set_state (app->playbin, GST_STATE_PLAYING);
	g_free (uri);
}

static gboolean
idle_play_alert (MyApp *app)
{
	char *fname;

	if (gconf_client_get_bool (app->client, CONFBASE"/sound_alert", NULL)) {
		fname = get_resource (app, "alert.wav");
		if (fname) {
			play_sound (app, fname);
			g_free (fname);
		} else {
			g_warning ("Couldn't find sound file %s", fname);
		}
	}
	return FALSE;
}

void
play_alert (MyApp *app)
{
	g_idle_add ((GSourceFunc) idle_play_alert, (gpointer) app);
}

static char *
get_current_btname (MyApp *app)
{
	char *name = NULL;
	if (bdaddr != NULL && bdaddr[0] != '\0') {
		name = gnomebt_controller_get_device_preferred_name (
				app->btctl, bdaddr);
	}
	if (!name)
		name = g_strdup (_("No Bluetooth device chosen."));

	return name;
}

static void
set_dependent_widget (MyApp *app, int conn_type, gboolean active)
{
	GtkWidget *w = NULL;
	switch (conn_type) {
		case CONNECTION_BLUETOOTH:
			/* only set sensitive if bluetooth available */
			if (app->btctl == NULL || phonemgr_utils_connection_is_supported (PHONEMGR_CONNECTION_BLUETOOTH) == FALSE)
				active = FALSE;
			w = GTK_WIDGET (glade_xml_get_widget (app->ui, "bluetooth_box"));
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
	int port = GPOINTER_TO_INT (
			g_object_get_data (G_OBJECT (widget), "port"));

	set_dependent_widget (app, port, active);
	if (active)
		conn_port = port;
}

static gboolean
message_dialog_reply (GladeXML *ui)
{
	MyApp *app;
	GtkWidget *dialog = glade_xml_get_widget (ui, "sms_dialog");
	GtkLabel *sender = GTK_LABEL (
			glade_xml_get_widget (ui, "senderlabel"));
	
	app = (MyApp *) g_object_get_data (G_OBJECT (dialog),
			"app");

	create_send_dialog (app, GTK_DIALOG (dialog),
			gtk_label_get_text (sender));

	return TRUE;
}

static void
set_btdevname (MyApp *app)
{
	GtkWidget *w = GTK_WIDGET (glade_xml_get_widget (app->ui, "btdevname"));
	char *c = get_current_btname (app);
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
	char *c;
	int ctype = gconf_client_get_int (app->client,
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

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "prefs_popup"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
			gconf_client_get_bool (app->client,
				CONFBASE"/popup_messages", NULL));

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "prefs_sound"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
			gconf_client_get_bool (app->client,
				CONFBASE"/sound_alert", NULL));

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
apply_prefs (MyApp *app)
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

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "prefs_popup"));
	gconf_client_set_bool (app->client,
			CONFBASE"/popup_messages",
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)), NULL);

	w = GTK_WIDGET (glade_xml_get_widget (app->ui, "prefs_sound"));
	gconf_client_set_bool (app->client,
			CONFBASE"/sound_alert",
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)), NULL);

	if (bdaddr != NULL && bdaddr[0] != '\0')
		gconf_client_set_string (app->client,
				CONFBASE"/bluetooth_addr", bdaddr, NULL);

	reconnect_phone (app);
}

static void
prefs_dialog_response (GtkWidget *dialog,
		       int response,
		       MyApp *app)
{
	if (response == GTK_RESPONSE_APPLY)
		apply_prefs (app);
	else if (response == GTK_RESPONSE_CLOSE)
		;
	else if (response == GTK_RESPONSE_HELP)
		;

	gtk_widget_hide (dialog);
}

static void
choose_bdaddr (MyApp *app, gpointer data)
{
    char *ret = NULL;
    GnomebtController *btctl;
    GnomebtChooser *btchooser;
    int result;

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

static gboolean
destroy_send_dialog (GtkDialog *dialog, GladeXML *ui)
{
	g_object_unref (ui);
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return TRUE;
}

static void
set_send_sensitivity (GtkWidget *w, GladeXML *ui)
{
	EPhoneEntry *entry;
	GtkTextView *view;
	GtkTextBuffer *buf;
	GtkWidget *sendbutton;
	GtkLabel *left;
	int l;
	char *work;
	char *text;

	left = GTK_LABEL (glade_xml_get_widget (ui, "charsleft"));
	view = GTK_TEXT_VIEW (glade_xml_get_widget (ui, "messagebody"));
	buf = gtk_text_view_get_buffer (view);
	entry = E_PHONE_ENTRY (glade_xml_get_widget (ui, "recipient"));
	sendbutton = GTK_WIDGET (glade_xml_get_widget (ui, "sendbutton"));

	l = gtk_text_buffer_get_char_count (buf);
	text = e_phone_entry_get_number (entry);

	gtk_widget_set_sensitive (sendbutton,
			l > 0 && l <= MAX_MESSAGE_LENGTH && text != NULL);
	g_free (text);

	if (l > MAX_MESSAGE_LENGTH) {
		gtk_label_set_text (left, _("Message too long!"));
	} else {
		int cl = MAX_MESSAGE_LENGTH - l;
		work = g_strdup_printf ("%d", cl);
		gtk_label_set_text (left, work);
		g_free (work);
	}
}

static void
phone_number_changed (GtkWidget *entry, char *phone_number, GladeXML *ui)
{
	set_send_sensitivity (entry, ui);
}

static void
send_message (GtkWidget *w, GladeXML *ui)
{
	GtkTextBuffer *buf;
	GtkTextView *view;
	EPhoneEntry *entry;
	GtkDialog *dialog;
	GtkTextIter s, e;
	char *number;

	MyApp *app = (MyApp *) g_object_get_data (G_OBJECT (ui), "app");

	dialog = GTK_DIALOG (glade_xml_get_widget (ui, "send_dialog"));
	view = GTK_TEXT_VIEW (glade_xml_get_widget (ui, "messagebody"));
	buf = gtk_text_view_get_buffer (view);
	entry = E_PHONE_ENTRY (glade_xml_get_widget (ui, "recipient"));

	gtk_text_buffer_get_start_iter (buf, &s);
	gtk_text_buffer_get_end_iter (buf, &e);
	number = e_phone_entry_get_number (entry);
	phonemgr_listener_queue_message (app->listener,
			number,
			gtk_text_buffer_get_text (buf, &s, &e, FALSE));
	g_free (number);

	gtk_widget_hide (GTK_WIDGET (dialog));
}

void
create_send_dialog (MyApp *app, GtkDialog *parent, const char *recip)
{
	GtkTextBuffer *buf;
	GtkTextView *view;
	GtkEntry *entry;
	GtkDialog *dialog;
	GladeXML *ui;
	GtkWidget *w;

	ui = get_ui (app, "send_dialog");

	dialog = GTK_DIALOG (glade_xml_get_widget (ui, "send_dialog"));

	if (parent) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
			GTK_WINDOW (parent));
	}
	
	view = GTK_TEXT_VIEW (glade_xml_get_widget (ui, "messagebody"));
	buf = gtk_text_view_get_buffer (view);
	gtk_text_buffer_set_text (buf, "", 0);
	entry = GTK_ENTRY (glade_xml_get_widget (ui, "recipient"));
	if (recip)
		gtk_entry_set_text (entry, recip);
	else
		gtk_entry_set_text (entry, "");

	set_send_sensitivity (NULL, ui);

	g_object_set_data (G_OBJECT (ui), "app", (gpointer) app);

	/* hook up signals */
	
	g_signal_connect (G_OBJECT (dialog), "delete-event",
			G_CALLBACK (gtk_widget_hide), (gpointer) dialog);

	g_signal_connect (G_OBJECT (dialog), "hide",
			G_CALLBACK (destroy_send_dialog), (gpointer) ui);

	w = GTK_WIDGET (glade_xml_get_widget (ui, "msgcancelbutton"));
	g_signal_connect_swapped (G_OBJECT (w), "clicked",
			G_CALLBACK (gtk_widget_hide), (gpointer) dialog);

	g_signal_connect (G_OBJECT (entry), "phone-changed",
			G_CALLBACK (phone_number_changed), (gpointer) ui);
	g_signal_connect (G_OBJECT (buf), "changed",
			G_CALLBACK (set_send_sensitivity), (gpointer) ui);

	w = GTK_WIDGET (glade_xml_get_widget (ui, "sendbutton"));
	g_signal_connect (G_OBJECT (w), "clicked",
			G_CALLBACK (send_message), (gpointer) ui);

	gtk_widget_show_all (GTK_WIDGET (dialog));
}

static void
initialise_dequeuer (GConfClient *client, guint cnxn_id,
		GConfEntry *entry, MyApp *app)
{
	if (gconf_client_get_bool (app->client,
				CONFBASE"/popup_messages", NULL)) {
		app->popup_cb = g_timeout_add (250,
				(GSourceFunc) dequeue_message, (gpointer) app);
	} else if (app->popup_cb) {
			g_source_remove (app->popup_cb);
			app->popup_cb = 0;
	}
}

void
ui_init (MyApp *app)
{
	GtkWidget *w;

	app->ui = get_ui (app, NULL);

	if (!app->ui)
		g_error ("Couldn't load user interface.");

	/* close button and windowframe close button just hide the
	   prefs panel */
	
	g_signal_connect_swapped (
			G_OBJECT (glade_xml_get_widget (app->ui, "prefs_dialog")),
			"delete-event", G_CALLBACK (gtk_widget_hide),
			G_OBJECT (glade_xml_get_widget (app->ui, "prefs_dialog")));

	/* response */
	g_signal_connect (glade_xml_get_widget (app->ui, "prefs_dialog"),
			  "response",
     			  G_CALLBACK (prefs_dialog_response),
			  app);

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

	/* set up popup on message */
	initialise_dequeuer (NULL, 0, NULL, app);
	gconf_client_notify_add (app->client,
			CONFBASE"/popup_messages",
			(GConfClientNotifyFunc) initialise_dequeuer,
			(gpointer) app,
			NULL, NULL);
}

void
ui_hide (MyApp *app)
{
	GtkWidget *w;
	GdkDisplay *display;

	w = glade_xml_get_widget (app->ui, "prefs_dialog");
	display = gtk_widget_get_display (w);
	gtk_widget_hide (w);

	w = glade_xml_get_widget (app->ui, "send_dialog");
	gtk_widget_hide (w);

	gdk_display_sync (display);
}

void
show_prefs_window (MyApp *app)
{
	GtkWidget *prefs = glade_xml_get_widget (app->ui, "prefs_dialog");
	populate_prefs (app);
	gtk_widget_show (prefs);
}

static char *
time_to_string (time_t time)
{
	char work[128];
	strftime (work, sizeof(work), "%c", gmtime (&time));
	return g_strdup (work);
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
	char *time;
	GladeXML *ui;
	GtkWidget *w;

	g_mutex_lock (app->message_mutex);
	ptr = g_list_first (app->messages);
	if (ptr) {
		app->messages = g_list_remove_link (app->messages, ptr);
	}
	g_mutex_unlock (app->message_mutex);

	if (! ptr) {
		return TRUE;
	}

	/* time to get on with displaying it */
	msg = (Message *) ptr->data;

	ui = get_ui (app, "sms_dialog");

	w = GTK_WIDGET (glade_xml_get_widget (ui, "sms_dialog"));
	g_signal_connect_swapped (G_OBJECT (w), "delete-event",
			G_CALLBACK (gtk_widget_destroy), (gpointer) w);

	g_object_set_data (G_OBJECT (w), "app", (gpointer) app);

	dialog = GTK_DIALOG (w);

	w = GTK_WIDGET (glade_xml_get_widget (ui, "okbutton"));
	g_signal_connect_swapped(G_OBJECT (w), "clicked",
			G_CALLBACK (gtk_widget_destroy), (gpointer) dialog);

	w = GTK_WIDGET (glade_xml_get_widget (ui, "replybutton"));
	g_signal_connect_swapped (G_OBJECT (w), "clicked",
			G_CALLBACK (message_dialog_reply), (gpointer) ui);

	l_sender = GTK_LABEL (glade_xml_get_widget (ui, "senderlabel"));
	l_sent = GTK_LABEL (glade_xml_get_widget (ui, "datelabel"));
	textview = GTK_TEXT_VIEW (glade_xml_get_widget (ui, "messagecontents"));

	buf = gtk_text_view_get_buffer (textview);
	gtk_text_buffer_set_text (buf, msg->message, strlen (msg->message));
	gtk_label_set_text (l_sender, msg->sender);

	time = time_to_string (msg->timestamp);
	gtk_label_set_text (l_sent, time);
	g_free (time);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	g_free (msg->sender);
	g_free (msg->message);
	g_free (msg);
	g_list_free_1 (ptr);

	set_icon_state (app);
	
	return TRUE;
}

