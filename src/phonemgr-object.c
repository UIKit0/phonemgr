
#include <stdlib.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "phonemgr-marshal.h"
#include "phonemgr-object.h"
#include "phone-manager-interface.h"
#include "app.h"

enum {
	NUMBER_BATTERIES_CHANGED,
	BATTERY_STATE_CHANGED,
	LAST_SIGNAL
};

static void phonemgr_object_init (PhonemgrObject *o);

static int phonemgr_object_signals[LAST_SIGNAL] = { 0 } ;

#define PHONEMGR_DBUS_SERVICE		"org.gnome.phone"
#define PHONEMGR_DBUS_INTERFACE		"org.gnome.phone.Manager"
#define PHONEMGR_DBUS_PATH		"/org/gnome/phone/Manager"

G_DEFINE_TYPE (PhonemgrObject, phonemgr_object, G_TYPE_OBJECT)

static void
phonemgr_object_class_init (PhonemgrObjectClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;

	phonemgr_object_signals[NUMBER_BATTERIES_CHANGED] =
		g_signal_new ("number-batteries-changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (PhonemgrObjectClass, number_batteries_changed),
			      NULL, NULL,
			      phonemgr_marshal_VOID__UINT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_UINT);
	phonemgr_object_signals[BATTERY_STATE_CHANGED] =
		g_signal_new ("battery-state-changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (PhonemgrObjectClass, battery_state_changed),
			      NULL, NULL,
			      phonemgr_marshal_VOID__UINT_UINT_BOOLEAN,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_UINT, G_TYPE_UINT, G_TYPE_BOOLEAN);

	dbus_g_object_type_install_info (phonemgr_object_get_type (),
					 &dbus_glib_phonemgr_object_object_info);
}

void
phonemgr_object_emit_number_batteries_changed (PhonemgrObject *o, guint num_batteries)
{
	if (num_batteries != 0)
		o->num_batteries = 1;
	else
		o->num_batteries = 0;

	g_signal_emit (G_OBJECT (o),
		       phonemgr_object_signals[NUMBER_BATTERIES_CHANGED],
		       0, num_batteries);
}

void
phonemgr_object_emit_battery_state_changed (PhonemgrObject *o, guint index, guint percentage, gboolean on_ac)
{
	o->percentage = percentage;
	o->on_ac = on_ac;

	g_signal_emit (G_OBJECT (o),
		       phonemgr_object_signals[BATTERY_STATE_CHANGED],
		       0, index, percentage, on_ac);
}

void
phonemgr_object_coldplug (PhonemgrObject *o)
{
	if (o->num_batteries == 0) {
		g_signal_emit (G_OBJECT (o),
			       phonemgr_object_signals[NUMBER_BATTERIES_CHANGED],
			       0, 0);
	} else {
		g_signal_emit (G_OBJECT (o),
			       phonemgr_object_signals[BATTERY_STATE_CHANGED],
			       0, 0, o->percentage, o->on_ac);
	}
}

static gboolean
phonemgr_object_register (DBusGConnection *connection,
		          GObject         *object)
{
	DBusGProxy *bus_proxy = NULL;
	GError *error = NULL;
	guint request_name_result;
	gboolean ret;

	bus_proxy = dbus_g_proxy_new_for_name (connection,
					       DBUS_SERVICE_DBUS,
					       DBUS_PATH_DBUS,
					       DBUS_INTERFACE_DBUS);

	ret = dbus_g_proxy_call (bus_proxy, "RequestName", &error,
				 G_TYPE_STRING, PHONEMGR_DBUS_SERVICE,
				 G_TYPE_UINT, 0,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, &request_name_result,
				 G_TYPE_INVALID);
	if (error) {
		g_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}
	if (ret == FALSE) {
		/* abort as the DBUS method failed */
		g_warning ("RequestName failed!");
		return FALSE;
	}

	/* free the bus_proxy */
	g_object_unref (G_OBJECT (bus_proxy));

	/* already running */
 	if (request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		return FALSE;
	}

	dbus_g_connection_register_g_object (connection, PHONEMGR_DBUS_PATH, object);
	return TRUE;
}

static void
phonemgr_object_init (PhonemgrObject *o)
{
	DBusGConnection *conn;
	GError *e = NULL;

	if (!(conn = dbus_g_bus_get (DBUS_BUS_SESSION, &e))) {
		g_warning ("Failed to get DBUS connection: %s", e->message);
		g_error_free (e);
	} else {
		phonemgr_object_register (conn, G_OBJECT (o));
		//FIXME connection leak?
	}
}

