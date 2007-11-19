/*
 * im-channel.c - PhoneyIMChannel source
 * Copyright © 2007 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2007 Will Thompson
 * Copyright (C) 2007 Collabora Ltd.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include <time.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>
#include <glib/gi18n.h>

#include <phonemgr-listener.h>

#include "im-channel.h"
#include "connection.h"
#include "debug.h"

/* properties */
enum
{
	PROP_CONNECTION = 1,
	PROP_OBJECT_PATH,
	PROP_CHANNEL_TYPE,
	PROP_HANDLE_TYPE,
	PROP_HANDLE,

	LAST_PROPERTY
};

typedef struct _PhoneyIMChannelPrivate
{
	PhoneyConnection *conn;
	char *object_path;
	guint handle;

	gboolean closed;
	gboolean dispose_has_run;
} PhoneyIMChannelPrivate;

#define PHONEY_IM_CHANNEL_GET_PRIVATE(o) \
	((PhoneyIMChannelPrivate *)o->priv)

static void channel_iface_init (gpointer, gpointer);
static void text_iface_init (gpointer, gpointer);
static void chat_state_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE(PhoneyIMChannel, phoney_im_channel, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL, channel_iface_init);
			G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_TEXT, text_iface_init);
			G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_IFACE, NULL);
			G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_CHAT_STATE,
					       chat_state_iface_init);
		       )

static void
phoney_im_channel_close (TpSvcChannel *iface,
			 DBusGMethodInvocation *context)
{
	PhoneyIMChannel *self = PHONEY_IM_CHANNEL (iface);
	PhoneyIMChannelPrivate *priv = PHONEY_IM_CHANNEL_GET_PRIVATE (self);

	DEBUG ("Called with TpSvcChannel: %p", iface);

	if (!priv->closed)
	{
		priv->closed = TRUE;
		//FIXME this is broken
		tp_svc_channel_emit_closed (iface);
	}

	tp_svc_channel_return_from_close(context);
}

static void
phoney_im_channel_get_channel_type (TpSvcChannel *iface,
				    DBusGMethodInvocation *context)
{
	tp_svc_channel_return_from_get_channel_type (context,
						     TP_IFACE_CHANNEL_TYPE_TEXT);
}

static void
phoney_im_channel_get_handle (TpSvcChannel *iface,
			      DBusGMethodInvocation *context)
{
	PhoneyIMChannel *self = PHONEY_IM_CHANNEL (iface);
	PhoneyIMChannelPrivate *priv = PHONEY_IM_CHANNEL_GET_PRIVATE (self);

	tp_svc_channel_return_from_get_handle (context, TP_HANDLE_TYPE_CONTACT,
					       priv->handle);
}

static void
phoney_im_channel_get_interfaces (TpSvcChannel *iface,
				  DBusGMethodInvocation *context)
{
	const char *no_interfaces[] = { NULL };
	const char *chat_state_ifaces[] =
	{ TP_IFACE_CHANNEL_INTERFACE_CHAT_STATE, NULL };
	tp_svc_channel_return_from_get_interfaces (context, chat_state_ifaces);
}

static void
channel_iface_init (gpointer g_iface, gpointer iface_data)
{
	TpSvcChannelClass *klass = (TpSvcChannelClass *)g_iface;

#define IMPLEMENT(x) tp_svc_channel_implement_##x (\
						   klass, phoney_im_channel_##x)
	IMPLEMENT(close);
	IMPLEMENT(get_channel_type);
	IMPLEMENT(get_handle);
	IMPLEMENT(get_interfaces);
#undef IMPLEMENT
}

static void
chat_state_iface_init (gpointer g_iface, gpointer iface_data)
{
	TpSvcChannelInterfaceChatStateClass *klass =
		(TpSvcChannelInterfaceChatStateClass *)g_iface;
}

void
phoney_im_channel_send (TpSvcChannelTypeText *channel,
			guint type,
			const gchar *text,
			DBusGMethodInvocation *context)
{
	PhoneyIMChannel *self = PHONEY_IM_CHANNEL (channel);
	PhoneyIMChannelPrivate *priv = PHONEY_IM_CHANNEL_GET_PRIVATE (self);
	TpHandleRepoIface *contact_handles;
	TpHandle handle;
	GError *error = NULL;
	PhonemgrListener *listener;
	char *message;
	const char *number;

	if (type >= NUM_TP_CHANNEL_TEXT_MESSAGE_TYPES) {
		DEBUG ("invalid message type %u", type);
		g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
			     "invalid message type: %u", type);
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return;
	}

	if (type == TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION) {
		/* Note to translators:
		 * This is supposed to be an action. eg. in English, this would be:
		 * /me does nothing
		 * German:
		 * /mich machts nichts
		 * French:
		 * /moi fait rien
		 */
		//FIXME
//		message = g_strconcat (_("/me"), " ", text, NULL);
		message = g_strconcat ("/me", " ", text, NULL);
	} else {
		message = g_strdup (text);
	}

	contact_handles = tp_base_connection_get_handles (TP_BASE_CONNECTION (priv->conn),
							  TP_HANDLE_TYPE_CONTACT);
	number = tp_handle_inspect (contact_handles, priv->handle);

	g_object_get (G_OBJECT (priv->conn), "listener", &listener, NULL);
	DEBUG ("Sending message to '%s': '%s'", number, message);
	phonemgr_listener_queue_message (listener, number, message, FALSE);
	//FIXME use tp_svc_channel_type_text_emit_send_error on errors
	tp_svc_channel_type_text_emit_sent (channel, time (NULL), type, message);

	g_free (message);

	tp_svc_channel_type_text_return_from_send (context);
}

static void
text_iface_init (gpointer g_iface, gpointer iface_data)
{
	TpSvcChannelTypeTextClass *klass = (TpSvcChannelTypeTextClass *)g_iface;

	tp_text_mixin_iface_init (g_iface, iface_data);
#define IMPLEMENT(x) tp_svc_channel_type_text_implement_##x (\
							     klass, phoney_im_channel_##x)
	IMPLEMENT(send);
#undef IMPLEMENT
}

static void
phoney_im_channel_get_property (GObject    *object,
				guint       property_id,
				GValue     *value,
				GParamSpec *pspec)
{
	PhoneyIMChannel *chan = PHONEY_IM_CHANNEL (object);
	PhoneyIMChannelPrivate *priv = PHONEY_IM_CHANNEL_GET_PRIVATE (chan);

	switch (property_id) {
	case PROP_OBJECT_PATH:
		g_value_set_string (value, priv->object_path);
		break;
	case PROP_CHANNEL_TYPE:
		g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_TEXT);
		break;
	case PROP_HANDLE_TYPE:
		g_value_set_uint (value, TP_HANDLE_TYPE_CONTACT);
		break;
	case PROP_HANDLE:
		g_value_set_uint (value, priv->handle);
		break;
	case PROP_CONNECTION:
		g_value_set_object (value, priv->conn);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
phoney_im_channel_set_property (GObject     *object,
				guint        property_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	PhoneyIMChannel *chan = PHONEY_IM_CHANNEL (object);
	PhoneyIMChannelPrivate *priv = PHONEY_IM_CHANNEL_GET_PRIVATE (chan);

	switch (property_id) {
	case PROP_OBJECT_PATH:
		g_free (priv->object_path);
		priv->object_path = g_value_dup_string (value);
		break;
	case PROP_HANDLE:
		/* we don't ref it here because we don't have access to the
		 * contact repo yet - instead we ref it in the constructor.
		 */
		priv->handle = g_value_get_uint (value);
		break;
	case PROP_HANDLE_TYPE:
		/* this property is writable in the interface, but not actually
		 * meaningfully changable on this channel, so we do nothing.
		 */
		break;
	case PROP_CONNECTION:
		priv->conn = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static GObject *
phoney_im_channel_constructor (GType type, guint n_props,
			       GObjectConstructParam *props)
{
	GObject *obj;
	TpHandleRepoIface *contact_handles;
	TpBaseConnection *conn;
	PhoneyIMChannelPrivate *priv;
	const char *recipient;
	DBusGConnection *bus;

	obj = G_OBJECT_CLASS (phoney_im_channel_parent_class)->
		constructor (type, n_props, props);
	priv = PHONEY_IM_CHANNEL_GET_PRIVATE(PHONEY_IM_CHANNEL(obj));
	conn = TP_BASE_CONNECTION(priv->conn);

	contact_handles = tp_base_connection_get_handles (conn,
							  TP_HANDLE_TYPE_CONTACT);
	tp_handle_ref (contact_handles, priv->handle);
	tp_text_mixin_init (obj, G_STRUCT_OFFSET (PhoneyIMChannel, text),
			    contact_handles);

	bus = tp_get_bus ();
	dbus_g_connection_register_g_object (bus, priv->object_path, obj);

	recipient = tp_handle_inspect(contact_handles, priv->handle);

	priv->closed = FALSE;
	priv->dispose_has_run = FALSE;

	return obj;
}

static void
phoney_im_channel_dispose (GObject *obj)
{
	PhoneyIMChannel *chan = PHONEY_IM_CHANNEL (obj);
	PhoneyIMChannelPrivate *priv = PHONEY_IM_CHANNEL_GET_PRIVATE (chan);

	if (priv->dispose_has_run)
		return;
	priv->dispose_has_run = TRUE;

	if (!priv->closed)
	{
		tp_svc_channel_emit_closed (obj);
		priv->closed = TRUE;
	}

	g_free (priv->object_path);
}

static void
phoney_im_channel_class_init (PhoneyIMChannelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec *param_spec;

	tp_text_mixin_class_init (object_class,
				  G_STRUCT_OFFSET(PhoneyIMChannelClass, text_class));

	g_type_class_add_private (klass, sizeof (PhoneyIMChannelPrivate));

	object_class->get_property = phoney_im_channel_get_property;
	object_class->set_property = phoney_im_channel_set_property;
	object_class->constructor = phoney_im_channel_constructor;
	object_class->dispose = phoney_im_channel_dispose;

	g_object_class_override_property (object_class, PROP_OBJECT_PATH,
					  "object-path");
	g_object_class_override_property (object_class, PROP_CHANNEL_TYPE,
					  "channel-type");
	g_object_class_override_property (object_class, PROP_HANDLE_TYPE,
					  "handle-type");
	g_object_class_override_property (object_class, PROP_HANDLE,
					  "handle");

	param_spec = g_param_spec_object ("connection", "PhoneyConnection object",
					  "Phoney connection object that owns this "
					  "IM channel object.",
					  PHONEY_TYPE_CONNECTION,
					  G_PARAM_CONSTRUCT_ONLY |
					  G_PARAM_READWRITE |
					  G_PARAM_STATIC_NICK |
					  G_PARAM_STATIC_BLURB);
	g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);
}

static void
phoney_im_channel_init (PhoneyIMChannel *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHONEY_TYPE_IM_CHANNEL,
						  PhoneyIMChannelPrivate);
}

