/*
 *  PhoneListener GObject wrapper.
 *
 *  Copyright (C) 2004  Edd Dumbill <edd@usefulinc.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
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

typedef struct _PhonemgrListener PhonemgrListener;
typedef struct _PhonemgrListenerClass PhonemgrListenerClass;

struct _PhonemgrListenerClass
{
  GObjectClass	parent_class;
  void (* message) (PhonemgrListener *bc, gchar *bdaddr, GTime timestamp, gchar *message);
  void (* status) (PhonemgrListener *bc, gint status);
};

/* Gtk housekeeping methods */

GType	phonemgr_listener_get_type	(void);
PhonemgrListener* phonemgr_listener_new	(void);

/* public methods */

gboolean phonemgr_listener_connect (PhonemgrListener *listener, gchar *device);
void phonemgr_listener_disconnect (PhonemgrListener *listener);
void phonemgr_listener_queue_message (PhonemgrListener *listener,
        const gchar *number, const gchar *message);
void phonemgr_listener_poll (PhonemgrListener *listener);
gboolean phonemgr_listener_connected (PhonemgrListener *listener);

/* status codes */

enum {
  PHONEMGR_LISTENER_IDLE,
  PHONEMGR_LISTENER_CONNECTING,
  PHONEMGR_LISTENER_CONNECTED,
  PHONEMGR_LISTENER_DISCONNECTING,
  PHONEMGR_LISTENER_ERROR
};

G_END_DECLS

#endif
