/*
 * connection-manager.c - PhoneyConnectionManager source
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

#include <glib.h>
#include <dbus/dbus-protocol.h>

#include "connection-manager.h"
#include "debug.h"

G_DEFINE_TYPE(PhoneyConnectionManager,
	      phoney_connection_manager,
	      TP_TYPE_BASE_CONNECTION_MANAGER)

typedef struct _PhoneyParams PhoneyParams;

struct _PhoneyParams {
	gchar *bdaddr;
};

static const TpCMParamSpec params[] = {
	{ "bdaddr", DBUS_TYPE_STRING_AS_STRING, G_TYPE_STRING,
		TP_CONN_MGR_PARAM_FLAG_REQUIRED, NULL,
		G_STRUCT_OFFSET(PhoneyParams, bdaddr), NULL, NULL },
	{ NULL, NULL, 0, 0, NULL, 0, NULL, NULL }
};

static void *
alloc_params (void)
{
	return g_new0 (PhoneyParams, 1);
}

static void
free_params (void *p)
{
	PhoneyParams *params = (PhoneyParams *)p;
	g_free (params->bdaddr);
	g_free (params);
}

const TpCMProtocolSpec phoney_protocols[] = {
  { "sms", params, alloc_params, free_params },
  { NULL, NULL }
};

PhoneyConnection *
phoney_connection_manager_get_phoney_connection (PhoneyConnectionManager *self,
						 const gchar *bdaddr)
{
	PhoneyConnection *conn;
	GList *l = self->connections;

	g_return_val_if_fail (bdaddr != NULL, NULL);

	while (l != NULL) {
		char *tmp;

		conn = l->data;
		g_object_get (G_OBJECT (conn), "bdaddr", &tmp, NULL);
		if(tmp != NULL && strcmp (tmp, bdaddr) == 0) {
			g_free (tmp);
			return conn;
		}
		g_free (tmp);
	}

	return NULL;
}

static void
connection_shutdown_finished_cb (TpBaseConnection *conn,
				 gpointer data)
{
	PhoneyConnectionManager *self = PHONEY_CONNECTION_MANAGER (data);
	char *bdaddr;

	g_object_get (G_OBJECT (conn), "bdaddr", &bdaddr, NULL);
	DEBUG ("removing connection for %s", bdaddr);
	g_free (bdaddr);

	self->connections = g_list_remove(self->connections, conn);
}

static TpBaseConnection *
_phoney_connection_manager_new_connection (TpBaseConnectionManager *base,
					   const gchar *proto,
					   TpIntSet *params_present,
					   void *parsed_params,
					   GError **error)
{
	PhoneyConnectionManager *cm = PHONEY_CONNECTION_MANAGER(base);
	PhoneyConnectionManagerClass *klass = PHONEY_CONNECTION_MANAGER_GET_CLASS (cm);
	PhoneyParams *params = (PhoneyParams *)parsed_params;
	PhoneyConnection *conn;

	/* Check if a connection already exists for that address, even if
	 * it's not connected */
	conn = phoney_connection_manager_get_phoney_connection (cm, params->bdaddr);
	if (conn != NULL) {
		DEBUG ("Already have a connection for '%s'", params->bdaddr);
		*error = g_error_new (TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
				      "Connection to '%s' already exists, not creating a new one",
				      params->bdaddr);
		return NULL;
	}

	conn = g_object_new (PHONEY_TYPE_CONNECTION,
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
phoney_connection_manager_class_init (PhoneyConnectionManagerClass *klass)
{
	TpBaseConnectionManagerClass *base_class =
		(TpBaseConnectionManagerClass *)klass;

	base_class->new_connection = _phoney_connection_manager_new_connection;
	base_class->cm_dbus_name = "phoney";
	base_class->protocol_params = phoney_protocols;
}

static void
phoney_connection_manager_init (PhoneyConnectionManager *self)
{
	DEBUG ("Initializing (PhoneyConnectionManager *)%p", self);
}

PhoneyConnectionManager *
phoney_connection_manager_get (void) {
	static PhoneyConnectionManager *manager = NULL;
	if (G_UNLIKELY(manager == NULL)) {
		manager = g_object_new (PHONEY_TYPE_CONNECTION_MANAGER, NULL);
	}
	g_assert (manager != NULL);
	return manager;
}
