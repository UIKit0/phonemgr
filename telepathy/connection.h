#ifndef __PHONEY_CONNECTION_H__
#define __PHONEY_CONNECTION_H__
/*
 * connection.h - PhoneyConnection header
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

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/presence-mixin.h>

#include "im-channel-factory.h"

G_BEGIN_DECLS

typedef struct _PhoneyConnection PhoneyConnection;
typedef struct _PhoneyConnectionClass PhoneyConnectionClass;

struct _PhoneyConnectionClass {
	TpBaseConnectionClass parent_class;
};

struct _PhoneyConnection {
	TpBaseConnection parent;

	PhoneyImChannelFactory *im_factory;

	gpointer priv;
};

#define ACCOUNT_GET_PHONEY_CONNECTION(account) \
	(PHONEY_CONNECTION ((account)->ui_data))
#define ACCOUNT_GET_TP_BASE_CONNECTION(account) \
	(TP_BASE_CONNECTION ((account)->ui_data))
#define PHONEY_CONNECTION_GET_PRPL_INFO(conn) \
	(PURPLE_PLUGIN_PROTOCOL_INFO (conn->account->gc->prpl))

const gchar *
phoney_connection_handle_inspect (PhoneyConnection *conn,
				  TpHandleType handle_type,
				  TpHandle handle);

GType phoney_connection_get_type (void);

/* TYPE MACROS */
#define PHONEY_TYPE_CONNECTION \
	(phoney_connection_get_type ())
#define PHONEY_CONNECTION(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PHONEY_TYPE_CONNECTION, \
				    PhoneyConnection))
#define PHONEY_CONNECTION_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PHONEY_TYPE_CONNECTION, \
				 PhoneyConnectionClass))
#define PHONEY_IS_CONNECTION(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PHONEY_TYPE_CONNECTION))
#define PHONEY_IS_CONNECTION_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PHONEY_TYPE_CONNECTION))
#define PHONEY_CONNECTION_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), PHONEY_TYPE_CONNECTION, \
				    PhoneyConnectionClass))

G_END_DECLS

#endif /* #ifndef __PHONEY_CONNECTION_H__*/
