
// gcc -Wall -o gnokii-identify `pkg-config --libs --cflags glib-2.0 gnokii`  gnokii-identify.c

#include <gnokii.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void
usage (char *name)
{
	g_print ("Usage: %s <BD addr>\n", name);
	exit (1);
}

static char *
write_config (char *addr, char *driver)
{
	char *str;

	str = g_strdup_printf ("[global]\n"
			"port = %s\n"
			"model = %s\n"
			"connection = bluetooth\n",
			addr, driver);
	return str;
}

static void
tell_driver (char *addr)
{
	char *config, **lines, model[64];
	gn_data data;
	struct gn_statemachine state;

	config = write_config (addr, "AT-HW");
	lines = g_strsplit (config, "\n", -1);

	if (gn_cfg_memory_read ((const char **)lines) < 0)
		g_warning ("gn_cfg_memory_read");
	g_strfreev (lines);

	memset (&state, 0, sizeof(struct gn_statemachine));

	if (gn_cfg_phone_load("", &state) < 0)
		g_warning ("gn_cfg_phone_load");

	gn_data_clear(&data);
	data.model = model;

	if (gn_gsm_initialise(&state) != GN_ERR_NONE)
		g_warning ("gn_gsm_initialise");

	if (gn_sm_functions(GN_OP_Identify, &data, &state) != GN_ERR_NONE)
		g_warning ("gn_sm_functions");

	gn_sm_functions(GN_OP_Terminate, NULL, &state);

	g_print ("model: %s\n", model);

	g_free (config);
}

int main (int argc, char **argv)
{
	if (argc < 2)
		usage (argv[0]);

	tell_driver (argv[1]);

	return 0;
}

