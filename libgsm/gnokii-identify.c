
// gcc -Wall -o gnokii-identify `pkg-config --libs --cflags glib-2.0 gnokii`  gnokii-identify.c

#include <gnokii.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "phonemgr-utils.h"

static void
usage (char *name)
{
	g_print ("Usage: %s <BD addr>\n", name);
	exit (1);
}

static void
tell_driver (char *addr)
{
	GError *error = NULL;
	PhonemgrState *phone_state;
	char model[64];

	phone_state = phonemgr_utils_connect (addr, NULL, &error);
	if (phone_state == NULL) {
		g_warning ("Couldn't connect to the '%s' phone: %s", addr, PHONEMGR_CONDERR_STR(error));
		if (error != NULL)
			g_error_free (error);
		return;
	}

	memset (model, 0, sizeof(model));
	phone_state->data.model = model;

	if (gn_sm_functions(GN_OP_Identify, &phone_state->data, &phone_state->state) != GN_ERR_NONE)
		g_warning ("gn_sm_functions");

	phonemgr_utils_disconnect (phone_state);
	phonemgr_utils_free (phone_state);

	g_print ("model: %s\n", model);
}

int main (int argc, char **argv)
{
	if (argc < 2)
		usage (argv[0]);

	tell_driver (argv[1]);

	return 0;
}

