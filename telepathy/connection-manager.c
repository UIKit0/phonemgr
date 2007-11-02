/*
 * connection-manager.c - SmsConnectionManager source
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

#include <glib.h>
#include <dbus/dbus-protocol.h>

#include "connection-manager.h"
#include "debug.h"

G_DEFINE_TYPE(SmsConnectionManager,
    sms_connection_manager,
    TP_TYPE_BASE_CONNECTION_MANAGER)

typedef struct _SmsParams SmsParams;

struct _SmsParams {
    gchar *bdaddr;
};

static const TpCMParamSpec params[] = {
  { "bdaddr", DBUS_TYPE_STRING_AS_STRING, G_TYPE_STRING,
    TP_CONN_MGR_PARAM_FLAG_REQUIRED, NULL,
    G_STRUCT_OFFSET(SmsParams, bdaddr), NULL, NULL },
};

static void *
alloc_params (void)
{
  return g_new0 (SmsParams, 1);
}

static void
free_params (void *p)
{
    SmsParams *params = (SmsParams *)p;
    g_free (params->bdaddr);

    g_free (params);
}

static const TpCMProtocolSpec protocol = {
	"sms",
	params,
	alloc_params,
	free_params
};

static TpCMProtocolSpec *
get_protocols (SmsConnectionManagerClass *klass)
{
    TpCMProtocolSpec *protocols;

    protocols = g_slice_alloc0 (sizeof (TpCMProtocolSpec) * (1 + 1));
    protocols[0] = protocol;

    return protocols;
}

SmsConnection *
sms_connection_manager_get_sms_connection (SmsConnectionManager *self,
                                           const gchar *bdaddr)
{
    SmsConnection *hc;
    GList *l = self->connections;

    while (l != NULL) {
        hc = l->data;
        if(strcmp (hc->bdaddr, bdaddr) == 0) {
            return hc;
        }
    }

    return NULL;
}

static void
connection_shutdown_finished_cb (TpBaseConnection *conn,
                                 gpointer data)
{
    SmsConnectionManager *self = SMS_CONNECTION_MANAGER (data);

    self->connections = g_list_remove(self->connections, conn);
}

static TpBaseConnection *
_sms_connection_manager_new_connection (TpBaseConnectionManager *base,
                                         const gchar *proto,
                                         TpIntSet *params_present,
                                         void *parsed_params,
                                         GError **error)
{
    SmsConnectionManager *cm = SMS_CONNECTION_MANAGER(base);
    SmsConnectionManagerClass *klass = SMS_CONNECTION_MANAGER_GET_CLASS (cm);
    SmsParams *params = (SmsParams *)parsed_params;
    SmsConnection *conn = g_object_new (SMS_TYPE_CONNECTION,
                                         "protocol",        proto,
                                         "bdaddr",          params->bdaddr,
                                         NULL);

    cm->connections = g_list_prepend(cm->connections, conn);
    g_signal_connect (conn, "shutdown-finished",
                      G_CALLBACK (connection_shutdown_finished_cb),
                      cm);

    return (TpBaseConnection *) conn;
}

static void
sms_connection_manager_class_init (SmsConnectionManagerClass *klass)
{
    TpBaseConnectionManagerClass *base_class =
        (TpBaseConnectionManagerClass *)klass;

    base_class->new_connection = _sms_connection_manager_new_connection;
    base_class->cm_dbus_name = "sms";
    base_class->protocol_params = get_protocols (klass);
}

static void
sms_connection_manager_init (SmsConnectionManager *self)
{
    DEBUG ("Initializing (SmsConnectionManager *)%p", self);
}

SmsConnectionManager *
sms_connection_manager_get (void) {
    static SmsConnectionManager *manager = NULL;
    if (G_UNLIKELY(manager == NULL)) {
        manager = g_object_new (SMS_TYPE_CONNECTION_MANAGER, NULL);
    }
    g_assert (manager != NULL);
    return manager;
}
