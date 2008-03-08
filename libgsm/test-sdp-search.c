
#include <glib.h>
#include "phonemgr-utils.h"

int main (int argc, char **argv)
{
	char *address;
	int channel;

	address = argv[1];
	if (address == NULL) {
		g_printerr ("Usage: %s [bdaddr]\n", argv[0]);
		return 1;
	}

	channel = phonemgr_utils_get_serial_channel (address);
	if (channel < 0) {
		g_printerr ("Couldn't find channel for device '%s'\n", address);
		return 1;
	}

	g_print ("Found channel %d for device '%s'\n", channel, address);

	return 0;
}

