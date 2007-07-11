
#include <stdlib.h>
#include <glib.h>

#include "phonemgr-object.h"
#include "app.h"

int
main (int argc, char **argv)
{
	MyApp *app;
	PhonemgrObject *o;

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

	o = g_object_new (phonemgr_object_get_type (), NULL);

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
