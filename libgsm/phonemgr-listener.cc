/*
 * PhoneManager Listener
 * Copyright (C) 2003-2004 Edd Dumbill <edd@usefulinc.com>
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

#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include <assert.h>

#include <sigc++/sigc++.h>
#include "phonemgr-listener.h"
#include "phonemgr-listener-private.h"
#include "phonemgr-marshal.h"

static gpointer		 parent_class = NULL;

/* gtk object prototypes */

static void phonemgr_listener_class_init (PhonemgrListenerClass *klass);
static void phonemgr_listener_init (PhonemgrListener *bc);
static void phonemgr_listener_finalize(GObject *obj);

enum {
	MESSAGE_SIGNAL,
	STATUS_SIGNAL,
	LAST_SIGNAL
};

static gint phonemgr_listener_signals[LAST_SIGNAL] = { 0 } ;

/* PhonemgrListener functions */

GType
phonemgr_listener_get_type()
{
	static GType model_type = 0;
	if (!model_type) {
		GTypeInfo model_info =
		{
			sizeof(PhonemgrListenerClass),
			NULL, /* base init */
			NULL, /* base finalize */
			(GClassInitFunc) phonemgr_listener_class_init,
			NULL, /* class finalize */
			NULL, /* class data */
			sizeof(PhonemgrListener),
			1, /* n_preallocs */
			(GInstanceInitFunc) phonemgr_listener_init,
		};
		model_type = g_type_register_static(G_TYPE_OBJECT,
				"PhonemgrListener",  &model_info, (GTypeFlags)0);
	}
	return model_type;
}


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

static void
phonemgr_listener_emit_message (PhonemgrListener *bo, const gchar *sender,
		GTime sent, const gchar *message)
{
	g_signal_emit (G_OBJECT (bo),
		phonemgr_listener_signals[MESSAGE_SIGNAL],
		0, sender, sent, message);
}

static void
phonemgr_listener_emit_status (PhonemgrListener *bo, gint status)
{
	g_signal_emit (G_OBJECT (bo),
		phonemgr_listener_signals[STATUS_SIGNAL],
		0, status);
}

static void
on_phone_message (std::string sender,
        time_t sent, std::string msg, PhonemgrListener *listener)
{
    phonemgr_listener_emit_message (listener, sender.c_str (),
            (GTime)sent, msg.c_str());
}

static void
on_status_change (int status, PhonemgrListener *listener)
{
    /* an error causes a disconnect */
    if (status == PHONELISTENER_ERROR)
        listener->connected = FALSE;

    phonemgr_listener_emit_status (listener, status);
}

static void
phonemgr_listener_init (PhonemgrListener *l)
{
    l->listener = new PhoneListener ();
    l->listener->signal_notify().connect (SigC::bind<PhonemgrListener*>
            (SigC::slot(&on_phone_message), l));
    l->listener->signal_status().connect (SigC::bind<PhonemgrListener*>
            (SigC::slot(&on_status_change), l));
}

PhonemgrListener *
phonemgr_listener_new ()
{
    PhonemgrListener *l = PHONEMGR_LISTENER (
            g_object_new (phonemgr_listener_get_type(), NULL));
    return l;
}

static void
phonemgr_listener_finalize(GObject *obj)
{
	PhonemgrListener *listener;

	listener = PHONEMGR_LISTENER (obj);
    if (listener) {
        if (listener->connected)
            listener->listener->disconnect ();
        listener->messagecon.disconnect ();
        listener->statuscon.disconnect ();
        delete listener->listener;
    }

	G_OBJECT_CLASS(parent_class)->finalize(obj);
}


gboolean
phonemgr_listener_connect (PhonemgrListener *listener, gchar *device)
{
    if (listener->listener->connect (device)) {
        listener->connected = TRUE;
    } else {
        listener->connected = FALSE;
        g_warning ("Unable to connect to device %s", device);
    }

    return listener->connected;
}

void
phonemgr_listener_disconnect (PhonemgrListener *listener)
{
    if (listener->connected) {
        listener->listener->request_disconnect ();
        listener->connected = FALSE;
    } else {
        g_warning ("phonemgr_listener_disconnect: not connected");
    }
}

void
phonemgr_listener_queue_message (PhonemgrListener *listener,
        const gchar *number, const gchar *message)
{
    if (listener->connected)
        listener->listener->queue_outgoing_message (number, message);
    else
        g_warning ("phonemgr_listener_queue_message: not connected");
}

void
phonemgr_listener_poll (PhonemgrListener *listener)
{
    if (listener->connected)
        listener->listener->polled_loop ();
}

gboolean
phonemgr_listener_connected (PhonemgrListener *listener)
{
    return listener->connected;
}
