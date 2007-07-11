

#include <stdlib.h>
#include <glib.h>

#include "phonemgr-listener.h"

GMainLoop *loop;

static void
message (PhonemgrListener *listener, const char *sender,
		time_t timestamp, const char *message)
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

static char *call_statutes[] = {
	"PHONEMGR_LISTENER_CALL_INCOMING",
	"PHONEMGR_LISTENER_CALL_ONGOING",
	"PHONEMGR_LISTENER_CALL_IDLE",
	"PHONEMGR_LISTENER_CALL_UNKNOWN"
};

static void
call_status (PhonemgrListener *listener, PhonemgrListenerCallStatus status, const char *phone, const char *name, gpointer user_data)
{
	g_message ("Got call status %s from %s (%s)", call_statutes[status], phone, name);
}

static void
battery (PhonemgrListener *l, int percent, gboolean on_ac, gpointer user_data)
{
	g_message ("battery is %d%%, on %s", percent, on_ac ? "mains" : "battery");
}

static void
status (PhonemgrListener *listener, gint status, gpointer user_data)
{
	g_message ("Got status %s (%d)", statuses[status], status);
}

int
main (int argc, char **argv)
{
	GError *err = NULL;
	PhonemgrListener *listener;

	g_thread_init (NULL);
	g_type_init ();
	
	listener = phonemgr_listener_new ();

	if (!listener)
		g_error ("Couldn't make listener");

	g_signal_connect (G_OBJECT (listener), "message",
			G_CALLBACK (message), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "status",
			G_CALLBACK (status), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "call-status",
			  G_CALLBACK (call_status), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "battery",
			  G_CALLBACK (battery), (gpointer) listener);

	if (phonemgr_listener_connect (listener, "00:12:D2:79:B7:33", &err)) {
//	if (phonemgr_listener_connect (listener, "/dev/rfcomm0", &err)) {
		g_message ("Connected OK");

		/* phonemgr_listener_queue_message (listener, "1234567",
				"test message XXX"); */

		loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (loop);

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
