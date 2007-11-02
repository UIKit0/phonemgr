#ifndef __SMS_CONNECTION_MANAGER_H__
#define __SMS_CONNECTION_MANAGER_H__

/*
 * connection-manager.h - SmsConnectionManager header
 * Copyright (C) 2007 Will Thompson
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
#include <telepathy-glib/base-connection-manager.h>

#include "connection.h"

G_BEGIN_DECLS

typedef struct _SmsConnectionManager SmsConnectionManager;
typedef struct _SmsConnectionManagerClass SmsConnectionManagerClass;

struct _SmsConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;
};

struct _SmsConnectionManager {
    TpBaseConnectionManager parent;
    GList *connections;
};

GType sms_connection_manager_get_type (void);

/* TYPE MACROS */
#define SMS_TYPE_CONNECTION_MANAGER \
  (sms_connection_manager_get_type ())
#define SMS_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SMS_TYPE_CONNECTION_MANAGER, \
                              SmsConnectionManager))
#define SMS_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), SMS_TYPE_CONNECTION_MANAGER, \
                           SmsConnectionManagerClass))
#define SMS_IS_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), SMS_TYPE_CONNECTION_MANAGER))
#define SMS_IS_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), SMS_TYPE_CONNECTION_MANAGER))
#define SMS_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SMS_TYPE_CONNECTION_MANAGER, \
                              SmsConnectionManagerClass))

SmsConnectionManager *sms_connection_manager_get (void);

SmsConnection *
sms_connection_manager_get_sms_connection (SmsConnectionManager *self,
                                           const gchar *bdaddr);

G_END_DECLS

#endif /* #ifndef __SMS_CONNECTION_MANAGER_H__*/
