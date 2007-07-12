
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "app.h"

static gboolean
set_connection_device (MyApp *app)
{
	int ctype;
	char *dev = NULL;
	gboolean changed;

	ctype = gconf_client_get_int (app->client,
				      CONFBASE"/connection_type", NULL);
	changed = FALSE;

	switch (ctype) {
		case CONNECTION_BLUETOOTH:
			dev = gconf_client_get_string (app->client,
						       CONFBASE"/bluetooth_addr",
						       NULL);
			if (!dev || strlen (dev) != 17) {
				g_free (dev);
				dev = NULL;
			}
			break;
		case CONNECTION_SERIAL1:
			dev = g_strdup ("/dev/ttyS0");
			break;
		case CONNECTION_SERIAL2:
			dev = g_strdup ("/dev/ttyS1");
			break;
		case CONNECTION_IRCOMM:
			dev = g_strdup ("/dev/ircomm0");
			break;
		case CONNECTION_OTHER:
			dev = gconf_client_get_string (app->client,
					CONFBASE"/other_serial", NULL);
			break;
	}

	if (dev == NULL && app->devname != NULL)
		changed = TRUE;
	else if (dev != NULL && app->devname == NULL)
		changed = TRUE;
	else if (dev != NULL && app->devname != NULL && strcmp (dev, app->devname) != 0)
		changed = TRUE;

	if (app->devname)
		g_free (app->devname);
	app->devname = dev;

	g_message ("New connection device is %s (%s)",
		   dev ? dev : "empty",
		   changed ? "changed" : "not changed");

	return changed;
}

static gboolean
attempt_reconnect (MyApp *app)
{
	if (gconf_client_get_bool (app->client,
				CONFBASE"/auto_retry", NULL) &&
			! phonemgr_listener_connected (app->listener) &&
			! app->connecting) {
		g_message ("Auto-retrying the connection");
		reconnect_phone (app);
	}
	return TRUE;
}

static gpointer
connect_phone_thread (gpointer data)
{
	MyApp *app = (MyApp *)data;
	GError *err = NULL;

	gdk_threads_enter ();
	set_icon_state (app);
	gdk_threads_leave ();
	set_connection_device (app);
	app->reconnect = FALSE;
	if (app->devname) {
		g_message ("Connecting...");
		if (phonemgr_listener_connect (app->listener, app->devname, &err)) {
			/* translators: the '%s' will be substituted with '/dev/ttyS0'
			   or similar */
			g_message (_("Connected to device on %s"), app->devname);
		} else {
			/* the ERROR signal will have been emitted, so we don't
			   bother changing the icon ourselves at this point */
			/* translators: the '%s' will be substituted with '/dev/ttyS0'
			   or similar */
			g_message (_("Failed connection to device on %s"), app->devname);
			if (err != NULL)
				g_message ("Error message is %s", err->message);
		}
	} else {
		g_message ("No device!");
	}
	g_mutex_lock (app->connecting_mutex);
	app->connecting = FALSE;
	g_mutex_unlock (app->connecting_mutex);

	gdk_threads_enter ();
	set_icon_state (app);
	gdk_threads_leave ();

	g_message ("Exiting connect thread");
	return NULL;
}

static void
connect_phone (MyApp *app)
{
	g_mutex_lock (app->connecting_mutex);
	if (phonemgr_listener_connected (app->listener) == FALSE
	    && app->connecting == FALSE) {
		app->connecting = TRUE;
		g_mutex_unlock (app->connecting_mutex);
		/* we're neither connected, nor connecting */
		app->connecting_thread = g_thread_create (connect_phone_thread,
							  (gpointer) app,
							  FALSE, NULL);
	} else {
		g_message ("Can't connect twice");
		g_mutex_unlock (app->connecting_mutex);
	}
}

static gboolean
idle_connect_phone (MyApp *app)
{
	connect_phone (app);
	return FALSE;
}

static void
on_status (PhonemgrListener *listener, int status, MyApp *app)
{
	g_message ("Status %d", status);
	gdk_threads_enter ();

	switch (status) {
		case PHONEMGR_LISTENER_IDLE:
			g_message ("Disconnect complete");
			set_icon_state (app);
			if (app->reconnect)
				g_idle_add ((GSourceFunc)idle_connect_phone,
						(gpointer)app);
			break;
		case PHONEMGR_LISTENER_CONNECTING:
			g_message ("Making serial port connection");
			break;
		case PHONEMGR_LISTENER_CONNECTED:
			g_message ("Serial port connected");
			phonemgr_object_emit_number_batteries_changed (app->object, 1);
			break;
		case PHONEMGR_LISTENER_DISCONNECTING:
			g_message ("Closing serial port connection");
			phonemgr_object_emit_number_batteries_changed (app->object, 0);
			break;
		case PHONEMGR_LISTENER_ERROR:
			set_icon_state (app);
			g_message ("Connected error occurred.");
			break;
		default:
			break;
	}

	gdk_threads_leave ();
}

static void
on_message (PhonemgrListener *listener, char *sender,
		time_t timestamp, char *message, MyApp *app)
{
	Message *msg;

	/* this handler called from a normal poll: happens
	   on the main gtk thread */
	g_message ("Got message %s | %ld | %s", sender, (long)timestamp, message);
	msg = g_new0 (Message, 1);
	msg->sender = g_strdup (sender);
	msg->timestamp = timestamp;
	msg->message = g_strdup (message);

	g_mutex_lock (app->message_mutex);
	app->messages = g_list_append (app->messages, (gpointer) msg);
	g_mutex_unlock (app->message_mutex);

	set_icon_state (app);
	play_alert (app);
}

static void
on_battery (PhonemgrListener *listener, int percent, gboolean on_ac, MyApp *app)
{
	phonemgr_object_emit_battery_state_changed (app->object, 0, percent, on_ac);
}

void
reconnect_phone (MyApp *app)
{
	gboolean changed;

	changed = set_connection_device (app);

	if (changed && phonemgr_listener_connected (app->listener) != FALSE) {
		g_message ("Disconnecting...");
		app->reconnect = TRUE;
		app->disconnecting_thread =
			g_thread_create ((GThreadFunc) phonemgr_listener_disconnect,
				(gpointer) app->listener, TRUE, NULL);
	} else {
		connect_phone (app);
	}
}

void
free_connection (MyApp *app)
{
	g_source_remove (app->reconnector);
	if (phonemgr_listener_connected (app->listener))
		phonemgr_listener_disconnect (app->listener);
	g_mutex_free (app->connecting_mutex);
	g_mutex_free (app->message_mutex);
}

void
disconnect_signal_handlers (MyApp *app)
{
	/* disconnect signal handlers, or they'll become blocked on
	gdk_threads_enter: called when menu-item Quit is activated */
	g_signal_handler_disconnect ((gpointer) app->listener,
			app->status_cb);
	g_signal_handler_disconnect ((gpointer) app->listener,
			app->message_cb);
}

void
initialise_connection (MyApp *app)
{
	app->status_cb = g_signal_connect (G_OBJECT (app->listener), "status",
			G_CALLBACK (on_status), (gpointer) app);
	app->message_cb = g_signal_connect (G_OBJECT (app->listener), "message",
			G_CALLBACK (on_message), (gpointer) app);
	app->battery_cb = g_signal_connect (G_OBJECT (app->listener), "battery",
						G_CALLBACK (on_battery), (gpointer) app);
#if GLIB_CHECK_VERSION(2,13,0)
	app->reconnector = g_timeout_add_seconds (20, (GSourceFunc)attempt_reconnect,
						  (gpointer)app);
#else
	app->reconnector = g_timeout_add (20 * 1000, (GSourceFunc)attempt_reconnect,
					  (gpointer)app);
#endif

	gdk_threads_init ();
	app->connecting_mutex = g_mutex_new ();
	app->message_mutex = g_mutex_new ();
}

