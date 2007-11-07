/*
 * connection.c - SmsConnection source
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
//#include "defines.h"
#include "connection-manager.h"
#include "im-channel-factory.h"
#include "connection.h"
//#include "connection-aliasing.h"
//#include "connection-avatars.h"

enum
{
    PROP_BDADDR = 1,

    LAST_PROPERTY
};

G_DEFINE_TYPE(SmsConnection, sms_connection, TP_TYPE_BASE_CONNECTION);

typedef struct _SmsConnectionPrivate
{
    PhonemgrListener *listener;
} SmsConnectionPrivate;

#define SMS_CONNECTION_GET_PRIVATE(o) \
  ((SmsConnectionPrivate *)o->priv)

#define PC_GET_BASE_LISTENER(pc) \
    (SMS_CONNECTION_GET_PRIVATE(pc)->listener)

gchar *
sms_connection_get_unique_connection_name(TpBaseConnection *base)
{
	SmsConnection *self = SMS_CONNECTION(base);

	return g_strdup (self->bdaddr);
}

static gboolean
_sms_connection_start_connecting (TpBaseConnection *base,
                                   GError **error)
{
    SmsConnection *self = SMS_CONNECTION(base);
    SmsConnectionPrivate *priv = SMS_CONNECTION_GET_PRIVATE(self);

#if 0
    TpHandleRepoIface *contact_handles =
        tp_base_connection_get_handles (base, TP_HANDLE_TYPE_CONTACT);

    base->self_handle = tp_handle_ensure (contact_handles,
        purple_account_get_username (self->account), NULL, error);
    if (!base->self_handle)
        return FALSE;

    purple_account_set_enabled(self->account, UI_ID, TRUE);
    purple_account_connect(self->account);
#endif

    tp_base_connection_change_status(base, TP_CONNECTION_STATUS_CONNECTING,
                                     TP_CONNECTION_STATUS_REASON_REQUESTED);

    //FIXME that should probably be threaded
    phonemgr_listener_connect (priv->listener, self->bdaddr, NULL);
    tp_base_connection_change_status(base, TP_CONNECTION_STATUS_CONNECTED,
                                     TP_CONNECTION_STATUS_REASON_REQUESTED);

    return TRUE;
}

static gchar*
_contact_normalize (TpHandleRepoIface *repo,
                    const gchar *id,
                    gpointer context,
                    GError **error)
{
    SmsConnection *conn = SMS_CONNECTION (context);
    return g_strdup (conn->bdaddr);
}

static void
_sms_connection_create_handle_repos (TpBaseConnection *base,
				     TpHandleRepoIface *repos[NUM_TP_HANDLE_TYPES])
{
    repos[TP_HANDLE_TYPE_CONTACT] =
        tp_dynamic_handle_repo_new (TP_HANDLE_TYPE_CONTACT, _contact_normalize,
                                    base);
    /* repos[TP_HANDLE_TYPE_ROOM] = XXX MUC */
#if 0
    repos[TP_HANDLE_TYPE_GROUP] =
        tp_dynamic_handle_repo_new (TP_HANDLE_TYPE_GROUP, NULL, NULL);
    repos[TP_HANDLE_TYPE_LIST] =
        tp_static_handle_repo_new (TP_HANDLE_TYPE_LIST, list_handle_strings);
#endif
}

static GPtrArray *
_sms_connection_create_channel_factories (TpBaseConnection *base)
{
    SmsConnection *self = SMS_CONNECTION(base);
    GPtrArray *channel_factories = g_ptr_array_new ();

    self->im_factory = SMS_IM_CHANNEL_FACTORY (
        g_object_new (SMS_TYPE_IM_CHANNEL_FACTORY, "connection", self, NULL));
    g_ptr_array_add (channel_factories, self->im_factory);
#if 0
    self->contact_list = SMS_CONTACT_LIST (
        g_object_new (SMS_TYPE_CONTACT_LIST, "connection", self, NULL));
    g_ptr_array_add (channel_factories, self->contact_list);
#endif
    return channel_factories;
}

static void
_sms_connection_shut_down (TpBaseConnection *base)
{
    SmsConnection *self = SMS_CONNECTION(base);
    SmsConnectionPrivate *priv = SMS_CONNECTION_GET_PRIVATE(self);

    phonemgr_listener_disconnect (priv->listener);
}

static void
sms_connection_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    SmsConnection *self = SMS_CONNECTION (object);

    switch (property_id) {
        case PROP_BDADDR:
            g_value_set_string (value, self->bdaddr);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
sms_connection_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    SmsConnection *self = SMS_CONNECTION (object);

    switch (property_id) {
        case PROP_BDADDR:
            g_free (self->bdaddr);
            self->bdaddr = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static GObject *
sms_connection_constructor (GType type,
                             guint n_construct_properties,
                             GObjectConstructParam *construct_params)
{
    SmsConnection *self = SMS_CONNECTION (
            G_OBJECT_CLASS (sms_connection_parent_class)->constructor (
                type, n_construct_properties, construct_params));

    DEBUG ("Post-construction: (SmsConnection *)%p", self);

//    _create_account (self);

    return (GObject *)self;
}

static void
sms_connection_dispose (GObject *object)
{
    SmsConnection *self = SMS_CONNECTION(object);

    DEBUG ("disposing of (SmsConnection *)%p", self);

    G_OBJECT_CLASS (sms_connection_parent_class)->dispose (object);
}

static void
sms_connection_finalize (GObject *object)
{
    SmsConnection *self = SMS_CONNECTION (object);
    SmsConnectionPrivate *priv = SMS_CONNECTION_GET_PRIVATE(self);

    g_object_unref (priv->listener);
    self->priv = NULL;
    g_free (self->bdaddr);

    G_OBJECT_CLASS (sms_connection_parent_class)->finalize (object);
}

static void
sms_connection_class_init (SmsConnectionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    TpBaseConnectionClass *base_class = TP_BASE_CONNECTION_CLASS (klass);
    GParamSpec *param_spec;

    DEBUG ("Initializing (SmsConnectionClass *)%p", klass);

    g_type_class_add_private (klass, sizeof (SmsConnectionPrivate));
    object_class->get_property = sms_connection_get_property;
    object_class->set_property = sms_connection_set_property;
    object_class->constructor = sms_connection_constructor;
    object_class->dispose = sms_connection_dispose;
    object_class->finalize = sms_connection_finalize;

    base_class->create_handle_repos = _sms_connection_create_handle_repos;
    base_class->create_channel_factories = _sms_connection_create_channel_factories;
    base_class->get_unique_connection_name =
        sms_connection_get_unique_connection_name;
    base_class->start_connecting = _sms_connection_start_connecting;
    base_class->shut_down = _sms_connection_shut_down;

    param_spec = g_param_spec_string ("bdaddr", "Phone's Bluetooth address",
                                      "The Bluetooth address used by the phone.",
                                      NULL,
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_BLURB);
    g_object_class_install_property (object_class, PROP_BDADDR, param_spec);

//    sms_connection_aliasing_class_init (object_class);
//    sms_connection_avatars_class_init (object_class);
}

static void
sms_connection_init (SmsConnection *self)
{
    SmsConnectionPrivate *priv;

    DEBUG ("Initializing (SmsConnection *)%p", self);
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, SMS_TYPE_CONNECTION,
                                              SmsConnectionPrivate);
    priv = SMS_CONNECTION_GET_PRIVATE(self);
    priv->listener = phonemgr_listener_new (FALSE);
}

const gchar *
sms_connection_handle_inspect (SmsConnection *conn,
                                TpHandleType handle_type,
                                TpHandle handle)
{
    TpBaseConnection *base_conn = TP_BASE_CONNECTION (conn);
    TpHandleRepoIface *handle_repo =
        tp_base_connection_get_handles (base_conn, handle_type);
    g_assert (tp_handle_is_valid (handle_repo, handle, NULL));
    return tp_handle_inspect (handle_repo, handle);
}
