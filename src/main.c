
#include <stdlib.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "phonemgr-marshal.h"
#include "phonemgr-battery-interface.h"
#include "app.h"

typedef struct _PhonemgrObject PhonemgrObject;
typedef struct _PhonemgrObjectClass PhonemgrObjectClass;

struct _PhonemgrObjectClass {
	GObjectClass  parent_class;
	void (* number_batteries_changed) (PhonemgrObject *o, guint num_batteries);
	void (* battery_state_changed) (PhonemgrObject *o,
					guint battery_index,
					guint percentage,
					gboolean on_ac);
};

struct _PhonemgrObject {
	GObject parent;
};

enum {
	NUMBER_BATTERIES_CHANGED,
	BATTERY_STATE_CHANGED,
	LAST_SIGNAL
};

static void phonemgr_object_init (PhonemgrObject *o);

static int phonemgr_object_signals[LAST_SIGNAL] = { 0 } ;

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
}

static void
phonemgr_object_init (PhonemgrObject *o)
{
}

int
main (int argc, char **argv)
{
	MyApp *app;
	DBusGConnection *conn;
	PhonemgrObject *o;
	GError *e = NULL;

	g_thread_init (NULL);

	app = g_new0 (MyApp, 1);

	app->program = gnome_program_init (
			PACKAGE,
			VERSION,
			LIBGNOMEUI_MODULE,
			argc, argv,
			GNOME_PARAM_APP_DATADIR, "..",
			GNOME_PARAM_APP_DATADIR, DATA_DIR,
			GNOME_PARAM_NONE);

	gconf_init (argc, argv, NULL);

	dbus_g_object_type_install_info (phonemgr_object_get_type (),
					 &dbus_glib_phonemgr_object_object_info);

	if (!(conn = dbus_g_bus_get (DBUS_BUS_SESSION, &e))) {
		g_warning ("Failed to get DBUS connection: %s", e->message);
		g_error_free (e);
		exit (1);
	}

	o = g_object_new (phonemgr_object_get_type (), NULL);
	dbus_g_connection_register_g_object (conn, "/org/gnome/totem/Battery", G_OBJECT (o));

	app->client = gconf_client_get_default ();
	gconf_client_add_dir (app->client, CONFBASE,
			GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	app->btctl = gnomebt_controller_new ();
	app->listener = phonemgr_listener_new ();

	if (! app->listener)
		g_error (_("Couldn't create phone listener."));

	initialise_connection (app);

	ui_init (app);
	icon_init (app);
	construct_menu (app);
	tray_icon_init (app);

	reconnect_phone (app);
	gtk_main ();

	tray_icon_hide (app);
	ui_hide (app);
	free_connection (app);
	g_object_unref (app->listener);
	g_object_unref (app->btctl);
	g_object_unref (app->client);
	g_object_unref (o);

	return 0;
}
