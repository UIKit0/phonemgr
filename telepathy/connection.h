#ifndef __SMS_CONNECTION_H__
#define __SMS_CONNECTION_H__
/*
 * connection.h - SmsConnection header
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

//#include "contact-list.h"

G_BEGIN_DECLS

typedef struct _SmsConnection SmsConnection;
typedef struct _SmsConnectionClass SmsConnectionClass;

struct _SmsConnectionClass {
    TpBaseConnectionClass parent_class;
};

struct _SmsConnection {
    TpBaseConnection parent;

    char *bdaddr;

//    SmsContactList *contact_list;

    gpointer priv;
};

#define ACCOUNT_GET_SMS_CONNECTION(account) \
    (SMS_CONNECTION ((account)->ui_data))
#define ACCOUNT_GET_TP_BASE_CONNECTION(account) \
    (TP_BASE_CONNECTION ((account)->ui_data))
#define SMS_CONNECTION_GET_PRPL_INFO(conn) \
    (PURPLE_PLUGIN_PROTOCOL_INFO (conn->account->gc->prpl))

const gchar *
sms_connection_handle_inspect (SmsConnection *conn,
                                TpHandleType handle_type,
                                TpHandle handle);

GType sms_connection_get_type (void);

/* TYPE MACROS */
#define SMS_TYPE_CONNECTION \
  (sms_connection_get_type ())
#define SMS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SMS_TYPE_CONNECTION, \
                              SmsConnection))
#define SMS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), SMS_TYPE_CONNECTION, \
                           SmsConnectionClass))
#define SMS_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), SMS_TYPE_CONNECTION))
#define SMS_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), SMS_TYPE_CONNECTION))
#define SMS_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SMS_TYPE_CONNECTION, \
                              SmsConnectionClass))

G_END_DECLS

#endif /* #ifndef __SMS_CONNECTION_H__*/
