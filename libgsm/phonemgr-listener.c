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

typedef struct {
	PhonemgrListenerCallStatus status;
	char *number;
	char *name;
} PhoneMgrCall;

typedef struct {
	float batterylevel;
	gn_power_source powersource;
} PhoneMgrBattery;

typedef struct {
	int type;
	union {
		gn_sms *message;
		PhoneMgrCall *call;
		PhoneMgrBattery *battery;
	};
} AsyncSignal;

struct _PhonemgrListener
{
	GObject object;

	GThread *thread;
	GAsyncQueue *queue;
	GMutex *mutex;

	PhonemgrState *phone_state;

	char *driver;

	/* The number of unread messages, or the number
	 * of messages in the folder if unread reporting isn't
	 * supported */
	guint old_state;
	/* The default folder for new messages, we should use
	 * smsstatus.new_message_store instead, but nothing 
	 * sets it */
	gn_memory_type default_mem;

	/* The previous call status */
	gn_call_status prev_call_status;

	/* Battery info */
	float batterylevel;
	gn_power_source powersource;

	guint connected : 1;
	guint terminated : 1;

	/* Whether the driver supports reporting unread through
	 * GN_OP_GetSMSStatus */
	guint supports_unread : 1;

	/* Whether the driver supports GN_OP_OnSMS */
	guint supports_sms_notif : 1;
};

static void phonemgr_listener_class_init (PhonemgrListenerClass *klass);
static void phonemgr_listener_init (PhonemgrListener *bc);
static void phonemgr_listener_finalize (GObject *obj);

#ifndef DUMMY
static void phonemgr_listener_thread (PhonemgrListener *l);
static void phonemgr_listener_sms_notification_real_poll (PhonemgrListener *l);
#endif

enum {
	MESSAGE_SIGNAL,
	STATUS_SIGNAL,
	CALL_STATUS_SIGNAL,
	BATTERY_SIGNAL,
	LAST_SIGNAL
};

static int phonemgr_listener_signals[LAST_SIGNAL] = { 0 } ;

G_DEFINE_TYPE (PhonemgrListener, phonemgr_listener, G_TYPE_OBJECT)

static void
phonemgr_listener_class_init (PhonemgrListenerClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	object_class = (GObjectClass*) klass;

	phonemgr_listener_signals[MESSAGE_SIGNAL] =
		g_signal_new ("message",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (PhonemgrListenerClass, message),
			      NULL, NULL,
			      phonemgr_marshal_VOID__STRING_ULONG_STRING,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_STRING);

	phonemgr_listener_signals[STATUS_SIGNAL] =
		g_signal_new ("status",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (PhonemgrListenerClass, status),
			      NULL, NULL,
			      phonemgr_marshal_VOID__UINT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_UINT);

	phonemgr_listener_signals[CALL_STATUS_SIGNAL] =
		g_signal_new ("call-status",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (PhonemgrListenerClass, call_status),
			      NULL, NULL,
			      phonemgr_marshal_VOID__UINT_STRING_STRING,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);

	phonemgr_listener_signals[BATTERY_SIGNAL] =
		g_signal_new ("battery",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (PhonemgrListenerClass, battery),
			      NULL, NULL,
			      phonemgr_marshal_VOID__INT_BOOLEAN,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_INT, G_TYPE_BOOLEAN);

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
phonemgr_listener_emit_status (PhonemgrListener *l, PhonemgrListenerStatus status)
{
	g_signal_emit (G_OBJECT (l),
		       phonemgr_listener_signals[STATUS_SIGNAL],
		       0, status);
}

static void
phonemgr_listener_emit_call_status (PhonemgrListener *l,
				    PhonemgrListenerCallStatus status,
				    const char *phone,
				    const char *name)
{
	g_signal_emit (G_OBJECT (l),
		       phonemgr_listener_signals[CALL_STATUS_SIGNAL],
		       0, status, phone, name);
}

static void
phonemgr_listener_emit_battery (PhonemgrListener *l,
				int percent,
				gboolean on_battery)
{
	g_signal_emit (G_OBJECT (l),
		       phonemgr_listener_signals[BATTERY_SIGNAL],
		       0, percent, on_battery);
}

static void
phonemgr_listener_emit_message (PhonemgrListener *l, gn_sms *message)
{
	time_t time;
	char *text, *sender, *origtext;

	text = NULL;

	//FIXME use the encoding as coming out of the txt message
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

PhonemgrListener *
phonemgr_listener_new (void)
{
	return PHONEMGR_LISTENER (g_object_new (PHONEMGR_TYPE_LISTENER, NULL));
}

#ifndef DUMMY
static void
phonemgr_listener_init (PhonemgrListener *l)
{
	l->queue = g_async_queue_new ();
	l->mutex = g_mutex_new ();
	l->old_state = 0;
	l->supports_unread = TRUE;
	l->default_mem = GN_MT_IN;
	l->driver = NULL;
	l->batterylevel = 1;
	l->powersource = GN_PS_BATTERY;
}

static void
phonemgr_listener_finalize(GObject *obj)
{
	PhonemgrListener *l;

	l = PHONEMGR_LISTENER (obj);
	if (l != NULL) {
		if (l->connected)
			phonemgr_listener_disconnect (l);
		//FIXME empty the queue of its stuff
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

	if (strcmp (l->driver, PHONEMGR_DEFAULT_DRIVER) == 0) {
		/* The AT driver doesn't support reading the number of unread messages:
		 * http://bugzilla.gnome.org/show_bug.cgi?id=330773 */
		l->supports_unread = FALSE;
		/* The AT driver doesn't support the GN_MT_IN memory type, as per atgen.c */
		l->default_mem = GN_MT_ME;
	}

	g_message ("Using driver '%s'", l->driver);
	l->connected = TRUE;

	phonemgr_listener_emit_status (l, PHONEMGR_LISTENER_CONNECTED);

	l->thread = g_thread_create ((GThreadFunc) phonemgr_listener_thread,
			l, TRUE, NULL);

	return l->connected;
}

static gboolean
phonemgr_listener_push (PhonemgrListener *l)
{
	AsyncSignal *signal;

	g_return_if_fail (l->connected != FALSE);

	signal = g_async_queue_try_pop (l->queue);
	if (signal == NULL)
		return FALSE;

	if (signal->type == MESSAGE_SIGNAL) {
		g_message ("emitting message");
		phonemgr_listener_emit_message (l, signal->message);
		g_free (signal->message);
	} else if (signal->type == CALL_STATUS_SIGNAL) {
		g_message ("emitting call status");
		phonemgr_listener_emit_call_status (l, signal->call->status,
						    signal->call->number,
						    signal->call->name);
		g_free (signal->call->number);
		g_free (signal->call->name);
		g_free (signal->call);
	} else if (signal->type == BATTERY_SIGNAL) {
		g_message ("emitting battery");
		phonemgr_listener_emit_battery (l,
						(int) signal->battery->batterylevel,
						signal->battery->powersource == GN_PS_BATTERY);
		g_free (signal->battery);
	} else {
		g_assert_not_reached ();
	}

	g_free (signal);

	return FALSE;
}

static gn_error
phonemgr_listener_new_sms_cb (gn_sms *message, struct gn_statemachine *state, void *user_data)
{
	PhonemgrListener *l = (PhonemgrListener *) user_data;
	AsyncSignal *signal;

	signal = g_new0 (AsyncSignal, 1);
	signal->type = MESSAGE_SIGNAL;
	/* The message is allocated on the stack in the driver, so copy it */
	signal->message = g_memdup (message, sizeof (gn_sms));

	g_async_queue_push (l->queue, signal);
	g_idle_add ((GSourceFunc) phonemgr_listener_push, l);

	return GN_ERR_NONE;
}

static void
phonemgr_listener_call_status (PhonemgrListener *l, gn_call_status call_status, const char *number, const char *name)
{
	PhonemgrListenerCallStatus status;
	PhoneMgrCall *call;
	AsyncSignal *signal;

	if (call_status == l->prev_call_status)
		return;

	status = PHONEMGR_LISTENER_CALL_UNKNOWN;

	switch (call_status) {
	case GN_CALL_Idle:
		status = PHONEMGR_LISTENER_CALL_IDLE;
		break;
	case GN_CALL_Incoming:
		status = PHONEMGR_LISTENER_CALL_INCOMING;
		break;
	case GN_CALL_RemoteHangup:
	case GN_CALL_LocalHangup:
	case GN_CALL_Held:
	case GN_CALL_Resumed:
		break;
	case GN_CALL_Established:
		status = PHONEMGR_LISTENER_CALL_ONGOING;
		break;
	default:
		break;
	}

	if (status == PHONEMGR_LISTENER_CALL_UNKNOWN)
		return;
	l->prev_call_status = call_status;

	call = g_new0 (PhoneMgrCall, 1);
	call->status = status;
	call->number = g_strdup (number);
	call->name = g_strdup (name);

	signal = g_new0 (AsyncSignal, 1);
	signal->type = CALL_STATUS_SIGNAL;
	signal->call = call;

	g_async_queue_push (l->queue, signal);
	g_idle_add ((GSourceFunc) phonemgr_listener_push, l);
}

static void
phonemgr_listener_new_call_cb (gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *user_data)
{
	PhonemgrListener *l = (PhonemgrListener *) user_data;
	PhonemgrListenerCallStatus status;

	/* We should ignore things that aren't the first call, but we have no idea of what
	 * call ID the drivers might be using */

	gn_call_notifier (call_status, call_info, state);

	phonemgr_listener_call_status (l, call_status, call_info->number, call_info->name);
}

static void
phonemgr_listener_sms_notification_soft_poll (PhonemgrListener *l)
{
	gn_sm_loop (1, &l->phone_state->state);
	/* Some phones may not be able to notify us, thus we give
	 * lowlevel chance to poll them */
	gn_sm_functions (GN_OP_PollSMS, &l->phone_state->data, &l->phone_state->state);
}

static void
phonemgr_listener_call_notification_poll (PhonemgrListener *l)
{
	gn_call *call;

	/* Don't call gn_sm_loop(), if the SMS notification already does it */
	if (l->supports_sms_notif == FALSE)
		gn_sm_loop (1, &l->phone_state->state);

	/* Check for active calls */
	gn_call_check_active (&l->phone_state->state);
	call = gn_call_get_active (0);
	if (call == NULL) {
		/* Call is NULL when it's GN_CALL_Idle */
		phonemgr_listener_call_status (l, GN_CALL_Idle, NULL, NULL);
	} else {
		phonemgr_listener_call_status (l, call->status, call->remote_number, call->remote_name);
	}
}

static void
phonemgr_listener_battery_poll (PhonemgrListener *l)
{
	float batterylevel = -1;
	gn_power_source powersource = -1;
	gn_battery_unit battery_unit = GN_BU_Arbitrary;

	(&l->phone_state->data)->battery_level = &batterylevel;
	(&l->phone_state->data)->power_source = &powersource;
	(&l->phone_state->data)->battery_unit = &battery_unit;

	if (gn_sm_functions(GN_OP_GetPowersource, &l->phone_state->data, &l->phone_state->state) != GN_ERR_NONE)
		powersource = GN_PS_BATTERY;

	/* Some drivers will use the same function for battery level and power source, so optimise */
	if (batterylevel == -1)
		if (gn_sm_functions(GN_OP_GetBatteryLevel, &l->phone_state->data, &l->phone_state->state) != GN_ERR_NONE)
			return;


	if (batterylevel != l->batterylevel || powersource != l->powersource) {
		AsyncSignal *signal;

		l->batterylevel = batterylevel;
		l->powersource = powersource;

		/* Probably not the best guess, but that's what xgnokii uses */
		if (battery_unit == GN_BU_Arbitrary)
			batterylevel *= 25;

		signal = g_new0 (AsyncSignal, 1);
		signal->type = BATTERY_SIGNAL;
		signal->battery = g_new0 (PhoneMgrBattery, 1);
		signal->battery->batterylevel = batterylevel;
		signal->battery->powersource = powersource;

		g_async_queue_push (l->queue, signal);
		g_idle_add ((GSourceFunc) phonemgr_listener_push, l);
	}

}

static void
phonemgr_listener_set_sms_notification (PhonemgrListener *l, gboolean state)
{
	if (state != FALSE) {
		/* Try to set up SMS notification using GN_OP_OnSMS */
		gn_data_clear (&l->phone_state->data);
		l->phone_state->data.on_sms = phonemgr_listener_new_sms_cb;
		l->phone_state->data.callback_data = l;
		if (gn_sm_functions (GN_OP_OnSMS, &l->phone_state->data, &l->phone_state->state) == GN_ERR_NONE) {
			l->supports_sms_notif = TRUE;
		}
	} else {
		if (l->supports_sms_notif == FALSE)
			return;
		/* Disable the SMS callback on exit */
		if (l->supports_sms_notif != FALSE) {
			l->phone_state->data.on_sms = NULL;
			gn_sm_functions (GN_OP_OnSMS, &l->phone_state->data, &l->phone_state->state);
		}
	}
}

static void
phonemgr_listener_set_call_notification (PhonemgrListener *l, gboolean state)
{
	if (state != FALSE) {
		/* Set up Call notification using GN_OP_SetCallNotification */
		gn_data_clear (&l->phone_state->data);
		l->phone_state->data.call_notification = phonemgr_listener_new_call_cb;
		l->phone_state->data.callback_data = l;
		gn_sm_functions (GN_OP_SetCallNotification, &l->phone_state->data, &l->phone_state->state);
	} else {
		/* Disable call notification */
		gn_data_clear (&l->phone_state->data);
		l->phone_state->data.call_notification = NULL;
		gn_sm_functions (GN_OP_SetCallNotification, &l->phone_state->data, &l->phone_state->state);
	}
}

static void
phonemgr_listener_thread (PhonemgrListener *l)
{
	g_mutex_lock (l->mutex);
	phonemgr_listener_set_sms_notification (l, TRUE);
	phonemgr_listener_set_call_notification (l, TRUE);
	g_mutex_unlock (l->mutex);

	while (l->terminated == FALSE) {
		if (g_mutex_trylock (l->mutex) != FALSE) {
			if (l->supports_sms_notif != FALSE)
				phonemgr_listener_sms_notification_soft_poll (l);
			else
				phonemgr_listener_sms_notification_real_poll (l);
			phonemgr_listener_call_notification_poll (l);
			phonemgr_listener_battery_poll (l);

			g_mutex_unlock (l->mutex);
			g_usleep (POLL_TIMEOUT);
		} else {
			g_usleep (TRYLOCK_TIMEOUT);
		}
	}

	g_mutex_lock (l->mutex);
	phonemgr_listener_set_sms_notification (l, FALSE);
	phonemgr_listener_set_call_notification (l, FALSE);
	g_mutex_unlock (l->mutex);

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
phonemgr_listener_set_time (PhonemgrListener *l,
			    time_t time)
{
	struct tm *t;
	gn_timestamp date;
	gn_error error;
	PhoneMgrError perr;

	t = localtime(&time);
	date.year = t->tm_year;
	date.month = t->tm_mon + 1;
	date.day = t->tm_mday;
	date.hour = t->tm_hour;
	date.minute = t->tm_min;
	date.second = t->tm_sec;
	free (t);

	/* Lock the phone */
	g_mutex_lock (l->mutex);

	/* Set the time and date */
	gn_data_clear(&l->phone_state->data);
	l->phone_state->data.datetime = &date;
	error = gn_sm_functions (GN_OP_SetDateTime, &l->phone_state->data, &l->phone_state->state);

	/* Unlock the phone */
	g_mutex_unlock (l->mutex);

	if (error != GN_ERR_NONE)
		g_warning ("Can't set date: %s", phonemgr_utils_gn_error_to_string (error, &perr));
}

static void
phonemgr_listener_sms_notification_real_poll (PhonemgrListener *l)
{
	gn_sms_status smsstatus = {0, 0, 0, 0};
	gn_sms_folder_list folderlist;
	gn_sms_folder folder;
	guint count, read, unread;
	int i, error, errcount;

	g_return_if_fail (l->connected != FALSE);

	errcount = count = read = unread = 0;

	gn_data_clear (&l->phone_state->data);

	/* Push battery status */
	//FIXME implement

	l->phone_state->data.sms_status = &smsstatus;

	if (gn_sm_functions(GN_OP_GetSMSStatus, &l->phone_state->data, &l->phone_state->state) != GN_ERR_NONE) {
		g_warning ("gn_sm_functions");
		return;
	}

	if (l->supports_unread != FALSE) {
		if (smsstatus.unread > 0 && l->old_state != smsstatus.unread) {
			l->old_state = smsstatus.unread;
			unread = smsstatus.unread;
		} else {
			l->old_state = smsstatus.unread;
			return;
		}
	} else {
		if (smsstatus.number != l->old_state) {
			if (smsstatus.number > l->old_state) {
				/* We have more messages now, so have a guess
				 * that the new ones are unread */
				unread = smsstatus.number - l->old_state;
			} else {
				/* We don't know how many are really unread, so
				 * we assume all of them are */
				l->old_state = smsstatus.number;
				unread = smsstatus.number;
			}
		} else {
			return;
		}
	}

	folder.folder_id = 0;
	l->phone_state->data.sms_folder = &folder;
	l->phone_state->data.sms_folder_list = &folderlist;

	i = smsstatus.number;

	//FIXME look in all of GN_OP_GetSMSFolders, but doesn't work for AT
	//FIXME use GN_OP_OnSMS as in smsreader() in gnokii-sms.c,
	//FIXME doesn't work on all devices though
	while (count < smsstatus.number && read < unread && i >= 0) {
		gn_sms *message;

		message = g_new0 (gn_sms, 1);
		memset (message, 0, sizeof (gn_sms));
		message->memory_type = l->default_mem;
		message->number = i;
		l->phone_state->data.sms = message;

		error = gn_sms_get (&l->phone_state->data, &l->phone_state->state);
		if (error == GN_ERR_NONE) {
			if (message->status == GN_SMS_Unread) {
				AsyncSignal *signal;

				g_message ("message pushed");

				signal = g_new0 (AsyncSignal, 1);
				signal->type = MESSAGE_SIGNAL;
				signal->message = message;
				g_async_queue_push (l->queue, signal);

				g_idle_add ((GSourceFunc) phonemgr_listener_push, l);

				read++;
			} else {
				g_free (message);
			}
			count++;
		} else {
			if (error == GN_ERR_EMPTYLOCATION) {
				error = GN_ERR_NONE;
				count++;
			} else if (error == GN_ERR_INVALIDMEMORYTYPE) {
				g_warning ("invalid memory type, can't continue");
				break;
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

static gboolean
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

	return TRUE;
}

gboolean
phonemgr_listener_connected (PhonemgrListener *listener)
{
	return listener->connected;
}

static void
phonemgr_listener_init (PhonemgrListener *l)
{
	g_timeout_add (50, (GSourceFunc) phonemgr_listener_poll, l);
}

static void
phonemgr_listener_finalize(GObject *obj)
{
}

#endif /* !DUMMY */
