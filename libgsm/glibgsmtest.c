

#include <stdlib.h>
#include <glib.h>

#include "phonemgr-listener.h"

GMainLoop *loop;

static gboolean
poll_listener (gpointer data)
{
	PhonemgrListener *listener = PHONEMGR_LISTENER (data);
	phonemgr_listener_poll (listener);
	return TRUE;
}

static void
message (PhonemgrListener *listener, const gchar *sender,
		GTime timestamp, const char *message)
{
	g_message ("Got message %s | %ld | %s", sender, (long)timestamp, message);
	g_main_loop_quit (loop);
}

static char *statuses[] = {
	"PHONEMGR_LISTENER_IDLE",
	"PHONEMGR_LISTENER_CONNECTING",
	"PHONEMGR_LISTENER_CONNECTED",
	"PHONEMGR_LISTENER_DISCONNECTING",
	"PHONEMGR_LISTENER_ERROR"
};

static void
status (PhonemgrListener *listener, gint status)
{
	g_message ("Got status %s (%d)", statuses[status], status);
}

int
main (int argc, char **argv)
{
	GError *err = NULL;
	PhonemgrListener *listener;
	guint timeout;

	g_thread_init (NULL);
	g_type_init ();
	
	listener = phonemgr_listener_new ();

	if (!listener)
		g_error ("Couldn't make listener");

	g_signal_connect (G_OBJECT (listener), "message",
			G_CALLBACK (message), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "status",
			G_CALLBACK (status), (gpointer) listener);

	if (phonemgr_listener_connect (listener, "00:60:57:1A:0A:6D", &err)) {
//	if (phonemgr_listener_connect (listener, "/dev/rfcomm0", &err)) {
		timeout = g_timeout_add (200, poll_listener,
				(gpointer) listener);
		g_message ("Connected OK");

		/* phonemgr_listener_queue_message (listener, "1234567",
				"test message XXX"); */

		loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (loop);

		g_source_remove (timeout);
		phonemgr_listener_disconnect (listener);
	} else {
		g_error ("Couldn't connect to the phone: %s\n",
				err ? err->message : "No reason");
		if (err)
			g_error_free (err);
	}

	g_object_unref (listener);

	return 0;
}
