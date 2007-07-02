
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

int main (int argc, char **argv)
{
	g_thread_init (NULL);
	g_type_init ();

	if (argc < 2)
		usage (argv[0]);

	phonemgr_utils_tell_driver (argv[1]);

	return 0;
}

