

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
		GTime timestamp, const gchar *message)
{
	g_message ("Got message %s | %ld | %s", sender, (long)timestamp, message);
	g_main_loop_quit (loop);
}

static void
status (PhonemgrListener *listener, gint status)
{
	g_message ("Got status %d", status);
}

int
main (int argc, char **argv)
{
	PhonemgrListener *listener;
	guint timeout;

	g_type_init ();
	
	listener = phonemgr_listener_new ();

	if (!listener)
		g_error ("Couldn't make listener");

	g_signal_connect (G_OBJECT (listener), "message",
			G_CALLBACK (message), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "status",
			G_CALLBACK (status), (gpointer) listener);
		
	if (phonemgr_listener_connect (listener, "/dev/rfcomm7")) {
		timeout = g_timeout_add (200, poll_listener,
				(gpointer) listener);
		g_message ("Connected OK");

		/* phonemgr_listener_queue_message (listener, "1234567",
				"test message"); */
		
		loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (loop);

		g_source_remove (timeout);
		phonemgr_listener_disconnect (listener);
	}

	g_object_unref (listener);

	return 0;
}
