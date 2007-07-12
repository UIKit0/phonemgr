
#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "phonemgr-object.h"
#include "app.h"

static char *bdaddr = NULL;

const GOptionEntry options[] = {
	{"identify", '\0', 0, G_OPTION_ARG_STRING, &bdaddr, N_("Show model name of a specific device"), NULL},
	{ NULL }
};

int
main (int argc, char **argv)
{
	MyApp *app;
	GOptionContext *context;
	GError *error = NULL;

	g_thread_init (NULL);

	app = g_new0 (MyApp, 1);

	context = g_option_context_new (N_("- Manage your mobile phone"));
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);

	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_error ("Couldn't parse options: %s", error->message);
		g_error_free (error);
		return 1;
	}

	if (bdaddr != NULL) {
		phonemgr_utils_tell_driver (bdaddr);
		return 0;
	}

	gconf_init (argc, argv, NULL);

	/* Setup the D-Bus object */
	app->object = g_object_new (phonemgr_object_get_type (), NULL);

	app->client = gconf_client_get_default ();
	gconf_client_add_dir (app->client, CONFBASE,
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	/* Setup the listener */
	app->listener = phonemgr_listener_new ();

	initialise_connection (app);

	icon_init (app);
	ui_init (app);
	construct_menu (app);
	tray_icon_init (app);

	reconnect_phone (app);
	gtk_main ();

	tray_icon_hide (app);
	ui_hide (app);
	free_connection (app);
	g_object_unref (app->listener);
	g_object_unref (app->client);
	g_object_unref (app->object);
	if (app->playbin)
		g_object_unref (app->playbin);

	return 0;
}
