/*
 * connection.c - PhoneyConnection source
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

#include <telepathy-glib/handle-repo-dynamic.h>
#include <telepathy-glib/handle-repo-static.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/errors.h>

#include <phonemgr-listener.h>

#include "debug.h"
#include "connection-manager.h"
#include "im-channel-factory.h"
#include "connection.h"

enum
{
	PROP_0,
	PROP_BDADDR,
	PROP_LISTENER
};

G_DEFINE_TYPE(PhoneyConnection, phoney_connection, TP_TYPE_BASE_CONNECTION);

typedef struct _PhoneyConnectionPrivate
{
	char *bdaddr;
	PhonemgrListener *listener;
} PhoneyConnectionPrivate;

#define PHONEY_CONNECTION_GET_PRIVATE(o) \
	((PhoneyConnectionPrivate *)o->priv)

#define PC_GET_BASE_LISTENER(pc) \
	(PHONEY_CONNECTION_GET_PRIVATE(pc)->listener)

gchar *
phoney_connection_get_unique_connection_name(TpBaseConnection *base)
{
	PhoneyConnection *self = PHONEY_CONNECTION(base);
	PhoneyConnectionPrivate *priv = PHONEY_CONNECTION_GET_PRIVATE(self);

	return g_strdup (priv->bdaddr);
}

static gboolean
_phoney_connection_start_connecting (TpBaseConnection *base,
				     GError **error)
{
	PhoneyConnection *self = PHONEY_CONNECTION(base);
	PhoneyConnectionPrivate *priv = PHONEY_CONNECTION_GET_PRIVATE(self);
	TpHandleRepoIface *contact_handles =
		tp_base_connection_get_handles (base, TP_HANDLE_TYPE_CONTACT);

	base->self_handle = tp_handle_ensure (contact_handles,
					      priv->bdaddr, NULL, error);
	if (!base->self_handle)
		return FALSE;

	tp_base_connection_change_status(base, TP_CONNECTION_STATUS_CONNECTING,
					 TP_CONNECTION_STATUS_REASON_REQUESTED);

	//FIXME that should probably be threaded
	phonemgr_listener_connect (priv->listener, priv->bdaddr, NULL);
	tp_base_connection_change_status(base, TP_CONNECTION_STATUS_CONNECTED,
					 TP_CONNECTION_STATUS_REASON_REQUESTED);

	return TRUE;
}

static void
_phoney_connection_create_handle_repos (TpBaseConnection *base,
					TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
	repos[TP_HANDLE_TYPE_CONTACT] =
		tp_dynamic_handle_repo_new (TP_HANDLE_TYPE_CONTACT, NULL, base);
}

static GPtrArray *
_phoney_connection_create_channel_factories (TpBaseConnection *base)
{
	PhoneyConnection *self = PHONEY_CONNECTION(base);
	GPtrArray *channel_factories = g_ptr_array_new ();

	self->im_factory = PHONEY_IM_CHANNEL_FACTORY (
	    g_object_new (PHONEY_TYPE_IM_CHANNEL_FACTORY, "connection", self, NULL));
	g_ptr_array_add (channel_factories, self->im_factory);
	return channel_factories;
}

static void
_phoney_connection_shut_down (TpBaseConnection *base)
{
	PhoneyConnection *self = PHONEY_CONNECTION(base);
	PhoneyConnectionPrivate *priv = PHONEY_CONNECTION_GET_PRIVATE(self);

	phonemgr_listener_disconnect (priv->listener);
}

static void
phoney_connection_get_property (GObject    *object,
				guint       property_id,
				GValue     *value,
				GParamSpec *pspec)
{
	PhoneyConnection *self = PHONEY_CONNECTION (object);
	PhoneyConnectionPrivate *priv = PHONEY_CONNECTION_GET_PRIVATE(self);

	switch (property_id) {
	case PROP_BDADDR:
		g_value_set_string (value, priv->bdaddr);
		break;
	case PROP_LISTENER:
		g_value_set_object (value, priv->listener);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
phoney_connection_set_property (GObject      *object,
				guint         property_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	PhoneyConnection *self = PHONEY_CONNECTION (object);
	PhoneyConnectionPrivate *priv = PHONEY_CONNECTION_GET_PRIVATE(self);

	switch (property_id) {
	case PROP_BDADDR:
		g_free (priv->bdaddr);
		priv->bdaddr = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static GObject *
phoney_connection_constructor (GType type,
			       guint n_construct_properties,
			       GObjectConstructParam *construct_params)
{
	PhoneyConnection *self = PHONEY_CONNECTION (
	    G_OBJECT_CLASS (phoney_connection_parent_class)->constructor (
		type, n_construct_properties, construct_params));

	DEBUG ("Post-construction: (PhoneyConnection *)%p", self);

	return (GObject *)self;
}

static void
phoney_connection_dispose (GObject *object)
{
	PhoneyConnection *self = PHONEY_CONNECTION(object);

	DEBUG ("disposing of (PhoneyConnection *)%p", self);

	G_OBJECT_CLASS (phoney_connection_parent_class)->dispose (object);
}

static void
phoney_connection_finalize (GObject *object)
{
	PhoneyConnection *self = PHONEY_CONNECTION (object);
	PhoneyConnectionPrivate *priv = PHONEY_CONNECTION_GET_PRIVATE(self);

	g_free (priv->bdaddr);
	g_object_unref (priv->listener);
	self->priv = NULL;

	G_OBJECT_CLASS (phoney_connection_parent_class)->finalize (object);
}

static void
phoney_connection_class_init (PhoneyConnectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TpBaseConnectionClass *base_class = TP_BASE_CONNECTION_CLASS (klass);
	GParamSpec *param_spec;

	DEBUG ("Initializing (PhoneyConnectionClass *)%p", klass);

	g_type_class_add_private (klass, sizeof (PhoneyConnectionPrivate));
	object_class->get_property = phoney_connection_get_property;
	object_class->set_property = phoney_connection_set_property;
	object_class->constructor = phoney_connection_constructor;
	object_class->dispose = phoney_connection_dispose;
	object_class->finalize = phoney_connection_finalize;

	base_class->create_handle_repos = _phoney_connection_create_handle_repos;
	base_class->create_channel_factories = _phoney_connection_create_channel_factories;
	base_class->get_unique_connection_name =
		phoney_connection_get_unique_connection_name;
	base_class->start_connecting = _phoney_connection_start_connecting;
	base_class->shut_down = _phoney_connection_shut_down;

	param_spec = g_param_spec_string ("bdaddr", "Phone's Bluetooth address",
					  "The Bluetooth address used by the phone.",
					  NULL,
					  G_PARAM_CONSTRUCT_ONLY |
					  G_PARAM_READWRITE |
					  G_PARAM_STATIC_NAME |
					  G_PARAM_STATIC_BLURB);
	g_object_class_install_property (object_class, PROP_BDADDR, param_spec);
	param_spec = g_param_spec_object ("listener", "Listener object",
					  "The listener object used from communicating with a phone.",
					  PHONEMGR_TYPE_LISTENER,
					  G_PARAM_READABLE |
					  G_PARAM_STATIC_NAME |
					  G_PARAM_STATIC_BLURB);
	g_object_class_install_property (object_class, PROP_LISTENER, param_spec);
}

static void
phoney_connection_init (PhoneyConnection *self)
{
	PhoneyConnectionPrivate *priv;

	DEBUG ("Initializing (PhoneyConnection *)%p", self);
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, PHONEY_TYPE_CONNECTION,
						  PhoneyConnectionPrivate);
	priv = PHONEY_CONNECTION_GET_PRIVATE(self);
	priv->bdaddr = NULL;
	priv->listener = phonemgr_listener_new (FALSE);
}

const gchar *
phoney_connection_handle_inspect (PhoneyConnection *conn,
				  TpHandleType handle_type,
				  TpHandle handle)
{
	TpBaseConnection *base_conn = TP_BASE_CONNECTION (conn);
	TpHandleRepoIface *handle_repo =
		tp_base_connection_get_handles (base_conn, handle_type);
	g_assert (tp_handle_is_valid (handle_repo, handle, NULL));
	return tp_handle_inspect (handle_repo, handle);
}

