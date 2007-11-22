/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2002-2004 Edd Dumbill <edd@usefulinc.com>
 * Copyright (C) 2005-2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>

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

static char *report_statuses[] = {
	"PHONEMGR_LISTENER_REPORT_DELIVERED",
	"PHONEMGR_LISTENER_REPORT_PENDING",
	"PHONEMGR_LISTENER_REPORT_FAILED_TEMPORARY",
	"PHONEMGR_LISTENER_REPORT_FAILED_PERMANENT"
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

static void
report_status (PhonemgrListener *l, char *phone, time_t timestamp, PhonemgrListenerReportStatus status, gpointer user_data)
{
	g_message ("Received delivery report status %s for %s", report_statuses[status], phone);
}

int
main (int argc, char **argv)
{
	GError *err = NULL;
	PhonemgrListener *listener;

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);
	g_type_init ();
	
	listener = phonemgr_listener_new (TRUE);

	g_signal_connect (G_OBJECT (listener), "message",
			G_CALLBACK (message), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "status",
			G_CALLBACK (status), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "call-status",
			  G_CALLBACK (call_status), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "battery",
			  G_CALLBACK (battery), (gpointer) listener);
	g_signal_connect (G_OBJECT (listener), "report-status",
			  G_CALLBACK (report_status), (gpointer) listener);

//	if (phonemgr_listener_connect (listener, "1", &err)) {
	if (phonemgr_listener_connect (listener, "00:12:D2:79:B7:33", &err)) {
//	if (phonemgr_listener_connect (listener, "/dev/rfcomm0", &err)) {
		g_message ("Connected OK");

		/* phonemgr_listener_queue_message (listener, "1234567",
				"test message XXX", TRUE); */

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
