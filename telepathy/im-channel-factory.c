/*
 * im-channel-factory.c - PhoneyImChannelFactory source
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

#include <string.h>

#include <telepathy-glib/channel-factory-iface.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/handle-repo.h>
#include <telepathy-glib/base-connection.h>

#include "debug.h"
#include "im-channel.h"
#include "im-channel-factory.h"
#include "connection.h"

typedef struct _PhoneyImChannelFactoryPrivate PhoneyImChannelFactoryPrivate;
struct _PhoneyImChannelFactoryPrivate {
	PhoneyConnection *conn;
	GHashTable *channels;
	gboolean dispose_has_run;
};
#define PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), PHONEY_TYPE_IM_CHANNEL_FACTORY, \
				     PhoneyImChannelFactoryPrivate))

static void phoney_im_channel_factory_iface_init (gpointer g_iface,
						  gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE(PhoneyImChannelFactory,
			phoney_im_channel_factory,
			G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_FACTORY_IFACE,
					       phoney_im_channel_factory_iface_init));

/* properties: */
enum {
	PROP_CONNECTION = 1,

	LAST_PROPERTY
};

static PhoneyIMChannel *get_im_channel (PhoneyImChannelFactory *self, TpHandle handle, gboolean *created);
static void set_im_channel_factory_listener (PhoneyImChannelFactory *fac, GObject *conn);
static void new_phoney (GObject *l, char *phone, time_t timestamp, char *message, gpointer data);

static void
phoney_im_channel_factory_init (PhoneyImChannelFactory *self)
{
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE(self);

	priv->channels = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

	priv->conn = NULL;
	priv->dispose_has_run = FALSE;
}

static void
phoney_im_channel_factory_dispose (GObject *object)
{
	PhoneyImChannelFactory *self = PHONEY_IM_CHANNEL_FACTORY (object);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE(self);

	if (priv->dispose_has_run)
		return;

	priv->dispose_has_run = TRUE;

	tp_channel_factory_iface_close_all (TP_CHANNEL_FACTORY_IFACE (object));
	g_assert (priv->channels == NULL);

	if (G_OBJECT_CLASS (phoney_im_channel_factory_parent_class)->dispose)
		G_OBJECT_CLASS (phoney_im_channel_factory_parent_class)->dispose (object);
}

static void
phoney_im_channel_factory_get_property (GObject    *object,
					guint       property_id,
					GValue     *value,
					GParamSpec *pspec)
{
	PhoneyImChannelFactory *fac = PHONEY_IM_CHANNEL_FACTORY (object);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (fac);

	switch (property_id) {
	case PROP_CONNECTION:
		g_value_set_object (value, priv->conn);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
phoney_im_channel_factory_set_property (GObject      *object,
					guint         property_id,
					const GValue *value,
					GParamSpec   *pspec)
{
	PhoneyImChannelFactory *fac = PHONEY_IM_CHANNEL_FACTORY (object);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (fac);

	switch (property_id) {
	case PROP_CONNECTION:
		priv->conn = g_value_get_object (value);
		set_im_channel_factory_listener (fac, G_OBJECT (priv->conn));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
phoney_im_channel_factory_class_init (PhoneyImChannelFactoryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec *param_spec;

	object_class->dispose = phoney_im_channel_factory_dispose;
	object_class->get_property = phoney_im_channel_factory_get_property;
	object_class->set_property = phoney_im_channel_factory_set_property;

	param_spec = g_param_spec_object ("connection", "PhoneyConnection object",
					  "Phoney connection object that owns this "
					  "IM channel factory object.",
					  PHONEY_TYPE_CONNECTION,
					  G_PARAM_CONSTRUCT_ONLY |
					  G_PARAM_READWRITE |
					  G_PARAM_STATIC_NICK |
					  G_PARAM_STATIC_BLURB);
	g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

	g_type_class_add_private (object_class,
				  sizeof(PhoneyImChannelFactoryPrivate));
}

static void
im_channel_closed_cb (PhoneyIMChannel *chan, gpointer user_data)
{
	PhoneyImChannelFactory *fac = PHONEY_IM_CHANNEL_FACTORY (user_data);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (fac);
	TpHandle contact_handle;

	if (priv->channels)
	{
		g_object_get (chan, "handle", &contact_handle, NULL);

		DEBUG ("removing channel with handle %d", contact_handle);

		g_hash_table_remove (priv->channels, GUINT_TO_POINTER (contact_handle));
	}
}

static PhoneyIMChannel *
new_im_channel (PhoneyImChannelFactory *self,
		guint handle)
{
	PhoneyImChannelFactoryPrivate *priv;
	TpBaseConnection *conn;
	PhoneyIMChannel *chan;
	char *object_path;

	g_assert (PHONEY_IS_IM_CHANNEL_FACTORY (self));

	priv = PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (self);
	conn = (TpBaseConnection *)priv->conn;

	g_assert (!g_hash_table_lookup (priv->channels, GUINT_TO_POINTER (handle)));

	object_path = g_strdup_printf ("%s/ImChannel%u", conn->object_path, handle);

	chan = g_object_new (PHONEY_TYPE_IM_CHANNEL,
			     "connection", priv->conn,
			     "object-path", object_path,
			     "handle", handle,
			     NULL);

	DEBUG ("Created IM channel with object path %s", object_path);

	g_signal_connect (chan, "closed", G_CALLBACK (im_channel_closed_cb), self);

	g_hash_table_insert (priv->channels, GUINT_TO_POINTER (handle), chan);

	tp_channel_factory_iface_emit_new_channel (self, (TpChannelIface *)chan,
						   NULL);

	g_free (object_path);

	return chan;
}

static PhoneyIMChannel *
get_im_channel (PhoneyImChannelFactory *self,
		TpHandle handle,
		gboolean *created)
{
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (self);
	PhoneyIMChannel *chan =
		g_hash_table_lookup (priv->channels, GUINT_TO_POINTER (handle));

	if (chan)
	{
		if (created)
			*created = FALSE;
	}
	else
	{
		chan = new_im_channel (self, handle);
		if (created)
			*created = TRUE;
	}
	g_assert (chan);
	return chan;
}

static void
phoney_im_channel_factory_iface_close_all (TpChannelFactoryIface *iface)
{
	PhoneyImChannelFactory *fac = PHONEY_IM_CHANNEL_FACTORY (iface);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (fac);
	GHashTable *tmp;

	DEBUG ("closing im channels");

	if (priv->conn)
	{
		GObject *listener;

		g_object_get (priv->conn, "listener", &listener, NULL);
		g_signal_handlers_disconnect_by_func(listener, new_phoney, fac);
	}
	if (priv->channels)
	{
		tmp = priv->channels;
		priv->channels = NULL;
		g_hash_table_destroy (tmp);
	}
}

static void
phoney_im_channel_factory_iface_connecting (TpChannelFactoryIface *iface)
{
	/* PhoneyImChannelFactory *self = PHONEY_IM_CHANNEL_FACTORY (iface); */
}

static void
phoney_im_channel_factory_iface_disconnected (TpChannelFactoryIface *iface)
{
	/* PhoneyImChannelFactory *self = PHONEY_IM_CHANNEL_FACTORY (iface); */
}

struct _ForeachData
{
	TpChannelFunc foreach;
	gpointer user_data;
};

static void
_foreach_slave (gpointer key, gpointer value, gpointer user_data)
{
	struct _ForeachData *data = (struct _ForeachData *) user_data;
	TpChannelIface *chan = TP_CHANNEL_IFACE (value);

	data->foreach (chan, data->user_data);
}

static void
phoney_im_channel_factory_iface_foreach (TpChannelFactoryIface *iface,
					 TpChannelFunc foreach,
					 gpointer user_data)
{
	PhoneyImChannelFactory *fac = PHONEY_IM_CHANNEL_FACTORY (iface);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (fac);
	struct _ForeachData data;

	data.user_data = user_data;
	data.foreach = foreach;

	g_hash_table_foreach (priv->channels, _foreach_slave, &data);
}

static TpChannelFactoryRequestStatus
phoney_im_channel_factory_iface_request (TpChannelFactoryIface *iface,
					 const gchar *chan_type,
					 TpHandleType handle_type,
					 guint handle,
					 gpointer request,
					 TpChannelIface **ret,
					 GError **error)
{
	PhoneyImChannelFactory *self = PHONEY_IM_CHANNEL_FACTORY (iface);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (self);
	TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
	    (TpBaseConnection *)priv->conn, TP_HANDLE_TYPE_CONTACT);
	PhoneyIMChannel *chan;
	TpChannelFactoryRequestStatus status;
	gboolean created;

	if (strcmp (chan_type, TP_IFACE_CHANNEL_TYPE_TEXT))
		return TP_CHANNEL_FACTORY_REQUEST_STATUS_NOT_IMPLEMENTED;

	if (handle_type != TP_HANDLE_TYPE_CONTACT)
		return TP_CHANNEL_FACTORY_REQUEST_STATUS_NOT_AVAILABLE;

	if (!tp_handle_is_valid (contact_repo, handle, error))
		return TP_CHANNEL_FACTORY_REQUEST_STATUS_ERROR;

	{
		const char *number;
		number = tp_handle_inspect (contact_repo, handle);
		DEBUG ("Creating new channel for number '%s'", number);
	}

	chan = get_im_channel (self, handle, &created);
	if (created)
	{
		status = TP_CHANNEL_FACTORY_REQUEST_STATUS_CREATED;
	}
	else
	{
		status = TP_CHANNEL_FACTORY_REQUEST_STATUS_EXISTING;
	}

	*ret = TP_CHANNEL_IFACE (chan);
	return status;
}

static void
phoney_im_channel_factory_iface_init (gpointer g_iface,
				      gpointer iface_data)
{
	TpChannelFactoryIfaceClass *klass = (TpChannelFactoryIfaceClass *) g_iface;

	klass->close_all = phoney_im_channel_factory_iface_close_all;
	klass->connecting = phoney_im_channel_factory_iface_connecting;
	klass->connected = NULL;
	klass->disconnected = phoney_im_channel_factory_iface_disconnected;
	klass->foreach = phoney_im_channel_factory_iface_foreach;
	klass->request = phoney_im_channel_factory_iface_request;
}

static void
new_phoney (GObject *l, char *phone, time_t timestamp, char *message, gpointer data)
{
	PhoneyImChannelFactory *self = PHONEY_IM_CHANNEL_FACTORY (data);
	PhoneyImChannelFactoryPrivate *priv =
		PHONEY_IM_CHANNEL_FACTORY_GET_PRIVATE (self);
	TpChannelTextMessageType type = TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL;
	PhoneyIMChannel *chan = NULL;
	TpHandle contact_handle;
	TpHandleRepoIface *contact_repo;

	contact_repo =
		tp_base_connection_get_handles (TP_BASE_CONNECTION (priv->conn), TP_HANDLE_TYPE_CONTACT);

	contact_handle = tp_handle_ensure (contact_repo, phone, NULL, NULL);

	chan = get_im_channel (self, contact_handle, NULL);

	tp_text_mixin_receive (G_OBJECT (chan), type, contact_handle,
			       timestamp, message);
	DEBUG ("channel %u: handled message %s phone: %s",
	       contact_handle, message, phone);
}

static void
set_im_channel_factory_listener (PhoneyImChannelFactory *fac, GObject *conn)
{
	GObject *listener;

	g_object_get (conn, "listener", &listener, NULL);
	g_signal_connect (G_OBJECT (listener), "message",
			  G_CALLBACK (new_phoney), fac);
}

