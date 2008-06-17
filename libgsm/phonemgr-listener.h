/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * PhoneManager Listener
 * Copyright (C) 2003-2004 Edd Dumbill <edd@usefulinc.com>
 * Copyright (C) 2005-2007 Bastien Nocera <hadess@hadess.net>
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

#ifndef __PHONEMGR_LISTENER_H__
#define __PHONEMGR_LISTENER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PHONEMGR_TYPE_LISTENER          (phonemgr_listener_get_type())
#define PHONEMGR_LISTENER(obj)          (G_TYPE_CHECK_INSTANCE_CAST (obj, PHONEMGR_TYPE_LISTENER, PhonemgrListener))
#define PHONEMGR_LISTENER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST (klass, PHONEMGR_TYPE_LISTENER, PhonemgrListenerClass))
#define PHONEMGR_IS_LISTENER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE (obj, PHONEMGR_TYPE_LISTENER))
#define PHONEMGR_IS_LISTENER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PHONEMGR_TYPE_LISTENER))
#define PHONEMGR_LISTENER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PHONEMGR_TYPE_LISTENER, PhonemgrListenerClass)
#define PHONEMGR_ERROR phonemgr_listener_error_quark ()

typedef enum {
	PHONEMGR_LISTENER_IDLE,
	PHONEMGR_LISTENER_CONNECTING,
	PHONEMGR_LISTENER_CONNECTED,
	PHONEMGR_LISTENER_DISCONNECTING,
	PHONEMGR_LISTENER_ERROR
} PhonemgrListenerStatus;

typedef enum {
	PHONEMGR_LISTENER_REPORT_DELIVERED,
	PHONEMGR_LISTENER_REPORT_PENDING,
	PHONEMGR_LISTENER_REPORT_FAILED_TEMPORARY,
	PHONEMGR_LISTENER_REPORT_FAILED_PERMANENT
} PhonemgrListenerReportStatus;

typedef enum {
	PHONEMGR_LISTENER_CALL_INCOMING,
	PHONEMGR_LISTENER_CALL_ONGOING,
	PHONEMGR_LISTENER_CALL_IDLE,
	PHONEMGR_LISTENER_CALL_UNKNOWN
} PhonemgrListenerCallStatus;

typedef enum {
	PHONEMGR_LISTENER_DATA_CONTACT,
	PHONEMGR_LISTENER_DATA_CALENDAR,
	PHONEMGR_LISTENER_DATA_TODO
} PhonemgrListenerDataType;

#define CALL_NAME_UNKNOWN "Unknown"
#define CALL_NAME_RESTRICTED "Withheld"

typedef struct _PhonemgrListener PhonemgrListener;
typedef struct _PhonemgrListenerClass PhonemgrListenerClass;

struct _PhonemgrListenerClass
{
  GObjectClass	parent_class;
  void (* message) (PhonemgrListener *l, char *phone, time_t timestamp, char *message);
  void (* report_status) (PhonemgrListener *l, char *phone, time_t timestamp, PhonemgrListenerReportStatus status);
  void (* status) (PhonemgrListener *l, PhonemgrListenerStatus status);
  void (* call_status) (PhonemgrListener *l, PhonemgrListenerCallStatus status, const char *phone, const char *name);
  void (* battery) (PhonemgrListener *l, int percent, gboolean on_ac);
  void (* network) (PhonemgrListener *l, int mcc, int mnc, int lac, int cid);
};

GQuark phonemgr_listener_error_quark	(void) G_GNUC_CONST;
GType	phonemgr_listener_get_type	(void);
PhonemgrListener* phonemgr_listener_new	(gboolean debug);
gboolean phonemgr_listener_connect	(PhonemgrListener *listener,
					 char *device,
					 GError **error);
void phonemgr_listener_disconnect	(PhonemgrListener *listener);
void phonemgr_listener_queue_message	(PhonemgrListener *listener,
					 const char *number,
					 const char *message,
					 gboolean deliver_report);
void phonemgr_listener_cancel_call	(PhonemgrListener *l);
void phonemgr_listener_answer_call	(PhonemgrListener *l);
void phonemgr_listener_set_time		(PhonemgrListener *l,
					 time_t time);

char ** phonemgr_listener_list_all_data
					(PhonemgrListener *l,
					 PhonemgrListenerDataType type);
char * phonemgr_listener_get_data	(PhonemgrListener *l,
					 PhonemgrListenerDataType type,
					 const char *dataid);
gboolean phonemgr_listener_delete_data	(PhonemgrListener *l,
					 PhonemgrListenerDataType type,
					 const char *dataid);
char * phonemgr_listener_put_data	(PhonemgrListener *l,
					 PhonemgrListenerDataType type,
					 const char *data);

gboolean phonemgr_listener_connected	(PhonemgrListener *listener);

/* Error types */
typedef enum {
	PHONEMGR_ERROR_COMMAND_FAILED,
	PHONEMGR_ERROR_UNKNOWN_MODEL,
	PHONEMGR_ERROR_INTERNAL_ERROR,
	PHONEMGR_ERROR_NOT_IMPLEMENTED,
	PHONEMGR_ERROR_NOT_SUPPORTED,
	PHONEMGR_ERROR_NO_LINK,
	PHONEMGR_ERROR_TIME_OUT,
	PHONEMGR_ERROR_NOT_READY
} PhoneMgrError;

G_END_DECLS

#endif
