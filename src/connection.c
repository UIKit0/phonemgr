
#include <stdlib.h>
#include <glib.h>

#include "app.h"

static gchar *
set_connection_device (MyApp *app)
{
	gint ctype = gconf_client_get_int (app->client,
			CONFBASE"/connection_type", NULL);
	gint portno;
	gchar *bdaddr;
	gchar *dev = NULL;

	bdaddr = gconf_client_get_string (app->client,
			CONFBASE"/bluetooth_addr", NULL);

	switch (ctype) {
		case CONNECTION_BLUETOOTH:
			portno = gnomebt_controller_get_rfcomm_port_by_service (
					app->btctl, bdaddr, 0x1103);
			if (portno < 0)
				portno = gnomebt_controller_connect_rfcomm_port_by_service (
						app->btctl, bdaddr, 0x1103);
			if (portno >= 0) {
				dev = g_strdup_printf ("/dev/rfcomm%d", portno);
				/* udev workaround: need to wait until udev device
				 is created */
				if (g_file_test ("/dev/.udev.tdb", G_FILE_TEST_EXISTS)) {
					while (! g_file_test (dev, G_FILE_TEST_EXISTS)) {
						g_message ("Waiting for udev device to appear");
						g_usleep (500000);
					}
				}
			} else {
				g_warning (_("Unable to obtain RFCOMM connection (%d)"),
						portno);
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

	if (bdaddr)
		g_free (bdaddr);

	if (dev) {
		if (app->devname)
			g_free (app->devname);
		app->devname = dev;
	}

	g_message ("New connection device is %s", dev);

	return dev;
}

static gboolean
poll_listener (PhonemgrListener *listener)
{
	phonemgr_listener_poll (listener);
	return TRUE;
}

static void
connect_phone (MyApp *app)
{
	set_connection_device (app);
	app->reconnect = FALSE;
	if (app->devname) {
		g_message ("Connecting...");
		if (phonemgr_listener_connect (app->listener, app->devname)) {
			g_message (_("Connected to device on %s"), app->devname);
			app->pollsource = g_timeout_add (200,
				(GSourceFunc)poll_listener,
				(gpointer) app->listener);
		} else {
			g_message (_("Failed to connect to device on %s"), app->devname);
		}
	} else {
		g_message ("No device!");
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

	switch (status) {
		case PHONEMGR_LISTENER_IDLE:
			g_message ("Disconnect complete");
			/* received when disconnected */
			g_source_remove (app->pollsource);
			app->pollsource = 0;
			tray_icon_set (app, ICON_IDLE);
			if (app->reconnect)
				g_idle_add ((GSourceFunc)idle_connect_phone, 
						(gpointer)app);
			break;
		case PHONEMGR_LISTENER_CONNECTING:
			tray_icon_set (app, ICON_CONNECTING);
			break;
		case PHONEMGR_LISTENER_ERROR:
			tray_icon_set (app, ICON_ERROR);
			break;
		default:
			break;
	}
}

static void
on_message (PhonemgrListener *listener, gchar *sender,
		GTime timestamp, gchar *message, MyApp *app)
{
	g_message ("Got message %s | %ld | %s", sender, (long)timestamp, message);
}

void
reconnect_phone (MyApp *app)
{
	if (phonemgr_listener_connected (app->listener)) {
		g_message ("Disconnecting...");
		app->reconnect = TRUE;
		phonemgr_listener_disconnect (app->listener);
	} else {
		connect_phone (app);
	}
}

void
free_connection (MyApp *app)
{
	if (phonemgr_listener_connected (app->listener))
		phonemgr_listener_disconnect (app->listener);
	if (app->pollsource)
		g_source_remove (app->pollsource);
}

void
initialise_connection (MyApp *app)
{
	g_signal_connect (G_OBJECT (app->listener), "status",
			G_CALLBACK (on_status), (gpointer) app);
	g_signal_connect (G_OBJECT (app->listener), "message",
			G_CALLBACK (on_message), (gpointer) app);
}
