/*
 * PhoneManager Listener
 * Copyright (C) 2003-2004 Edd Dumbill <edd@usefulinc.com>
 * Copyright (C) 2005 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <config.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h> 
#include <glib.h>
#include <glib-object.h>
#include <gnokii.h>

#include "phonemgr-listener.h"
#include "phonemgr-marshal.h"
#include "phonemgr-utils.h"

/* #define DUMMY 1 */
#define POLL_TIMEOUT 300
#define TRYLOCK_TIMEOUT 50

static gpointer		 parent_class = NULL;

struct _PhonemgrListener
{
	GObject object;

	GThread *thread;
	GAsyncQueue *queue;
	GMutex *mutex;
	gboolean terminated;

	PhonemgrState *phone_state;

	char *driver;
	guint old_state;

	gboolean connected;
};

static void phonemgr_listener_class_init (PhonemgrListenerClass *klass);
static void phonemgr_listener_init (PhonemgrListener *bc);
static void phonemgr_listener_finalize (GObject *obj);

#ifndef DUMMY
static void phonemgr_listener_thread (PhonemgrListener *l);
static void phonemgr_listener_poll_real (PhonemgrListener *l);
#endif

enum {
	MESSAGE_SIGNAL,
	STATUS_SIGNAL,
	LAST_SIGNAL
};

static gint phonemgr_listener_signals[LAST_SIGNAL] = { 0 } ;

G_DEFINE_TYPE (PhonemgrListener, phonemgr_listener, G_TYPE_OBJECT)

static void
phonemgr_listener_class_init (PhonemgrListenerClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_ref(G_TYPE_OBJECT);

	object_class=(GObjectClass*)klass;

	phonemgr_listener_signals[MESSAGE_SIGNAL] =
		g_signal_new ("message",
			G_OBJECT_CLASS_TYPE(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(PhonemgrListenerClass, message),
			NULL /* accu */, NULL,
			phonemgr_marshal_VOID__STRING_ULONG_STRING,
			G_TYPE_NONE,
			3,
			G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_STRING);

	phonemgr_listener_signals[STATUS_SIGNAL] =
		g_signal_new ("status",
			G_OBJECT_CLASS_TYPE(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(PhonemgrListenerClass, status),
			NULL /* accu */, NULL,
			phonemgr_marshal_VOID__UINT,
			G_TYPE_NONE,
			1,
			G_TYPE_UINT);

	object_class->finalize = phonemgr_listener_finalize;

	klass->message = NULL;
	klass->status = NULL;
}

GQuark
phonemgr_listener_error_quark (void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string ("phonemgr-error-quark");
	return q;
}

static void
phonemgr_listener_emit_status (PhonemgrListener *bo, gint status)
{
	g_signal_emit (G_OBJECT (bo),
		phonemgr_listener_signals[STATUS_SIGNAL],
		0, status);
}

PhonemgrListener *
phonemgr_listener_new ()
{
	PhonemgrListener *l = PHONEMGR_LISTENER (
			g_object_new (phonemgr_listener_get_type(), NULL));
	return l;
}

#ifndef DUMMY
static GTime
gn_timestamp_to_gtime (gn_timestamp stamp)
{
	GDate *date;
	GTime time = 0;
	int i;

	if (gn_timestamp_isvalid (stamp) == FALSE)
		return time;

	date = g_date_new_dmy (stamp.day, stamp.month, stamp.year);
	for (i = 1970; i < stamp.year; i++) {
		if (g_date_is_leap_year (i) != FALSE)
			time += 3600 * 24 * 366;
		else
			time += 3600 * 24 * 365;
	}
	time += g_date_get_day_of_year (date) * 3600 * 24;
	time += stamp.hour * 3600 + stamp.minute * 60 + stamp.second;

	g_free (date);

	return time;
}

static void
phonemgr_listener_emit_message (PhonemgrListener *l, gn_sms *message)
{
	GTime time;
	char *text, *sender, *origtext;

	text = NULL;

	origtext = (char *) message->user_data[0].u.text;
	if (g_utf8_validate (origtext, -1, NULL) == FALSE) {
		GError *err = NULL;

		text = g_convert (origtext,
				strlen (origtext),
				"utf-8", "iso-8859-1",
				NULL, NULL, &err);
		if (err != NULL) {
			g_warning ("Conversion error: %d %s",
					err->code, err->message);
			g_error_free (err);
			text = g_strdup (origtext);
		}
	}

	if (text == NULL)
		text = g_strdup (origtext);

	time = gn_timestamp_to_gtime (message->smsc_time);
	sender = g_strdup (message->remote.number);

	g_signal_emit (G_OBJECT (l),
			phonemgr_listener_signals[MESSAGE_SIGNAL],
			0, sender, time, text);

	g_free (text);
	g_free (sender);
}

static void
phonemgr_listener_init (PhonemgrListener *l)
{
	if (g_thread_supported () == FALSE)
		g_thread_init (NULL);

	l->queue = g_async_queue_new ();
	l->mutex = g_mutex_new ();
	l->old_state = 0;
	l->driver = NULL;
}

static void
phonemgr_listener_finalize(GObject *obj)
{
	PhonemgrListener *l;

	l = PHONEMGR_LISTENER (obj);
	if (l != NULL) {
		if (l->connected)
			phonemgr_listener_disconnect (l);
		g_async_queue_unref (l->queue);
		g_mutex_free (l->mutex);
	}

	G_OBJECT_CLASS (parent_class)->finalize(obj);
}

gboolean
phonemgr_listener_connect (PhonemgrListener *l, char *device, GError **error)
{
	g_return_val_if_fail (l->connected == FALSE, FALSE);
	g_return_val_if_fail (l->phone_state == NULL, FALSE);

	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_CONNECTING);

	l->phone_state = phonemgr_utils_connect (device, NULL, error);
	if (l->phone_state == NULL) {
		//FIXME
		return FALSE;
	}

	l->driver = phonemgr_utils_guess_driver (l->phone_state, device, error);
	if (l->driver == NULL) {
		//FIXME
		return FALSE;
	}

	/* Need a different driver? then reconnect */
	if (strcmp (l->driver, PHONEMGR_DEFAULT_DRIVER) != 0) {
		phonemgr_utils_disconnect (l->phone_state);
		phonemgr_utils_free (l->phone_state);
		l->phone_state = phonemgr_utils_connect (device, l->driver, error);
		if (l->phone_state == NULL) {
			//FIXME
			return FALSE;
		}
	}

	g_message ("Using driver '%s'", l->driver);
	l->connected = TRUE;

	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_CONNECTED);

	l->thread = g_thread_create ((GThreadFunc) phonemgr_listener_thread,
			l, TRUE, NULL);

	return l->connected;
}

static void
phonemgr_listener_thread (PhonemgrListener *l)
{
	while (l->terminated == FALSE) {
		if (g_mutex_trylock (l->mutex) != FALSE) {
			phonemgr_listener_poll_real (l);
			g_mutex_unlock (l->mutex);
			g_usleep (POLL_TIMEOUT);
		} else {
			g_usleep (TRYLOCK_TIMEOUT);
		}
	}
	g_thread_exit (NULL);
}

void
phonemgr_listener_disconnect (PhonemgrListener *l)
{
	g_return_if_fail (l->connected != FALSE);

	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_DISCONNECTING);

	l->terminated = TRUE;
	g_thread_join (l->thread);

	g_free (l->driver);
	l->driver = NULL;

	l->old_state = 0;
	l->connected = FALSE;
	//FIXME more to kill?
	phonemgr_utils_disconnect (l->phone_state);
	phonemgr_utils_free (l->phone_state);
	l->phone_state = NULL;

	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_IDLE);
}

void
phonemgr_listener_queue_message (PhonemgrListener *l,
		const char *number, const char *message)
{
	char *mstr;
	GError *err = NULL;
	gn_sms sms;
	gn_error error;

	g_return_if_fail (l->connected != FALSE);
	g_return_if_fail (number != NULL);
	g_return_if_fail (message != NULL);

	/* Lock the phone */
	g_mutex_lock (l->mutex);

	gn_data_clear(&l->phone_state->data);

	mstr = g_convert (message, strlen (message),
			"iso-8859-1", "utf-8",
			NULL, NULL, &err);

	g_message ("mstr '%s'", mstr);

	if (err != NULL) {
		g_warning ("Conversion error: %d %s", err->code, err->message);
		g_error_free (err);
		g_mutex_unlock (l->mutex);
		return;
	}

	gn_sms_default_submit(&sms);

	/* Set the destination number */
	g_strlcpy (sms.remote.number, number, sizeof(sms.remote.number));
	if (sms.remote.number[0] == '+')
		sms.remote.type = GN_GSM_NUMBER_International;
	else
		sms.remote.type = GN_GSM_NUMBER_Unknown;

	/* Get the SMS Center number */
	l->phone_state->data.message_center = g_new (gn_sms_message_center, 1);
	l->phone_state->data.message_center->id = 1;
	if (gn_sm_functions(GN_OP_GetSMSCenter, &l->phone_state->data, &l->phone_state->state) == GN_ERR_NONE) {
		g_strlcpy (sms.smsc.number, l->phone_state->data.message_center->smsc.number, sizeof (sms.smsc.number));
		sms.smsc.type = l->phone_state->data.message_center->smsc.type;
	}
	g_free (l->phone_state->data.message_center);

	/* Set the message data */
	sms.user_data[0].type = GN_SMS_DATA_Text;
	g_strlcpy ((char *) sms.user_data[0].u.text, mstr, strlen (mstr) + 1);
	sms.user_data[0].length = strlen (mstr);

	/* Set the message encoding */
	if (!gn_char_def_alphabet(sms.user_data[0].u.text))
		sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;
	sms.user_data[1].type = GN_SMS_DATA_None;
	g_free (mstr);

	l->phone_state->data.sms = &sms;

	/* Actually send the message */
	error = gn_sms_send (&l->phone_state->data, &l->phone_state->state);

	/* Unlock the phone */
	g_mutex_unlock (l->mutex);
}

void
phonemgr_listener_poll (PhonemgrListener *l)
{
	gn_sms *message;

	g_return_if_fail (l->connected != FALSE);

	message = g_async_queue_try_pop (l->queue);
	if (message != NULL) {
		g_message ("emitting message");
		phonemgr_listener_emit_message (l, message);
		g_free (message);
	}
}

static void
phonemgr_listener_poll_real (PhonemgrListener *l)
{
	gn_sms_status smsstatus = {0, 0, 0, 0};
	gn_sms_folder_list folderlist;
	gn_sms_folder folder;
	guint count, read;
	int i, error, errcount;

	errcount = count = read = 0;

	g_return_if_fail (l->connected != FALSE);

	gn_data_clear(&l->phone_state->data);
	l->phone_state->data.sms_status = &smsstatus;

	if (gn_sm_functions(GN_OP_GetSMSStatus, &l->phone_state->data, &l->phone_state->state) != GN_ERR_NONE) {
		g_warning ("gn_sm_functions");
		return;
	}

	if (smsstatus.unread > 0 && l->old_state != smsstatus.unread) {
		l->old_state = smsstatus.unread;
	} else {
		l->old_state = smsstatus.unread;
		return;
	}

	folder.folder_id = 0;
	l->phone_state->data.sms_folder = &folder;
	l->phone_state->data.sms_folder_list = &folderlist;

	i = smsstatus.number;

	while (count < smsstatus.number && read < smsstatus.unread) {
		gn_sms *message;

		message = g_new0 (gn_sms, 1);
		message->memory_type = GN_MT_IN;
		message->number = i;
		l->phone_state->data.sms = message;

		error = gn_sms_get (&l->phone_state->data, &l->phone_state->state);
		if (error == GN_ERR_NONE) {
			if (message->status == GN_SMS_Unread) {
				g_message ("message pushed");
				g_async_queue_push (l->queue, message);
				read++;
			} else {
				g_free (message);
			}
			count++;
		} else {
			g_free (message);

			if (error == GN_ERR_EMPTYLOCATION) {
				error = GN_ERR_NONE;
				count++;
			} else {
				errcount++;
			}

			if (errcount > 25)
				break;
		}
		i--;
	}
}

gboolean
phonemgr_listener_connected (PhonemgrListener *listener)
{
	return listener->connected;
}

#else /* !DUMMY */

gboolean
phonemgr_listener_connect (PhonemgrListener *l, char *device, GError **err)
{
	g_message ("[DUMMY] connecting to %s", device);
	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_CONNECTING);
	g_usleep (G_USEC_PER_SEC * 2);
	l->connected = TRUE;
	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_CONNECTED);

	return TRUE;
}

void
phonemgr_listener_disconnect (PhonemgrListener *l)
{
	g_message ("[DUMMY] disconnecting");
	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_DISCONNECTING);
	g_usleep (G_USEC_PER_SEC * 2);
	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_IDLE);
}

void
phonemgr_listener_queue_message (PhonemgrListener *listener,
        const char *number, const char *message)
{
	g_message ("[DUMMY] sending message to %s: %s", number, message);
}

/* We poll every 50 milliseconds, so this is the number of times
 * we need to go through poll() to wait for one second */
#define POLL_SECOND 1000 / 50

void
phonemgr_listener_poll (PhonemgrListener *l)
{
	static int i = 0;
	static int target = 5 * POLL_SECOND;

	i++;
	if (i == target) {
		char *sender = "+01234 567-89";
		GTimeVal time;
		char *text = "This is my test, this is my supa test";

		g_get_current_time (&time);

		g_message ("[DUMMY] receiving from %s: %s", sender, text);
		g_signal_emit (G_OBJECT (l),
				phonemgr_listener_signals[MESSAGE_SIGNAL],
				0, sender, time.tv_sec, text);
		target = 15 * POLL_SECOND;
	}
}

gboolean
phonemgr_listener_connected (PhonemgrListener *listener)
{
	return listener->connected;
}

static void
phonemgr_listener_init (PhonemgrListener *l)
{
}

static void
phonemgr_listener_finalize(GObject *obj)
{
}

#endif /* !DUMMY */
