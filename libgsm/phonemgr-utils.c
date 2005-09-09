/*
 * PhoneManager Utilities
 * Copyright (C) 2003-2004 Edd Dumbill <edd@usefulinc.com>
 * Copyright (C) 2005 Bastien Nocera <hadess@hadess.net>
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
#include <config.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h> 
#include <glib.h>
#include <glib-object.h>
#include <gnokii.h>

#include "phonemgr-utils.h"

/* This is the hash table containing information about driver <-> device
 * match */
static GHashTable *driver_device = NULL;
/* This is the hash table containing information about driver <-> model
 * match */
static GHashTable *driver_model = NULL;

void
phonemgr_utils_gn_statemachine_clear (struct gn_statemachine *state)
{
	memset (state, 0, sizeof(struct gn_statemachine));
}

static const char *
phonemgr_utils_gn_error_to_string (gn_error error)
{
	switch (error) {
	/* General codes */
	case GN_ERR_NONE:
		return NULL;
	case GN_ERR_FAILED:
		return "Command failed";
	case GN_ERR_UNKNOWNMODEL:
		return "Model specified isn't known/supported.";
	case GN_ERR_INVALIDSECURITYCODE:
		return "Invalid Security code.";
	case GN_ERR_INTERNALERROR:
		return "Problem occured internal to model specific code.";
	case GN_ERR_NOTIMPLEMENTED:
		return "Command called isn't implemented in model.";
	case GN_ERR_NOTSUPPORTED:
		return "Function not supported by the phone";
	case GN_ERR_USERCANCELED:
		return "User aborted the action.";
	case GN_ERR_UNKNOWN:
		return "Unknown error - well better than nothing";
	case GN_ERR_MEMORYFULL:
		return "The specified memory is full.";
	/* Statemachine */
	case GN_ERR_NOLINK:
		return "Couldn't establish link with phone.";
	case GN_ERR_TIMEOUT:
		return "Command timed out.";
	case GN_ERR_TRYAGAIN:
		return "Try again.";
	case GN_ERR_WAITING:
		return "Waiting for the next part of the message.";
	case GN_ERR_NOTREADY:
		return "Device not ready.";
	case GN_ERR_BUSY:
		return "Command is still being executed.";
	default:
		return "XXX Other error, FIXME";
	}
}

static gboolean
phonemgr_utils_is_bluetooth (const char *addr)
{
	return (g_file_test (addr, G_FILE_TEST_EXISTS) == FALSE);
}

char *
phonemgr_utils_write_config (const char *driver, const char *addr)
{
	if (phonemgr_utils_is_bluetooth (addr) == FALSE) {
		return g_strdup_printf ("[global]\n"
			"port = %s\n"
			"model = %s\n"
			"connection = serial\n", addr, driver);
	} else {
		return g_strdup_printf ("[global]\n"
				"port = %s\n"
				"model = %s\n"
				"connection = bluetooth\n", addr, driver);
	}
}

static char *
phonemgr_utils_driver_for_model (const char *model, const char *device)
{
	char *driver;

	driver = g_hash_table_lookup (driver_model, model);
	if (driver == NULL) {
		g_warning ("Model %s not supported natively", model);
		driver = g_strdup ("AT");
	} else {
		driver = g_strdup (driver);
	}

	/* Add it to the list if it's a bluetooth device */
	//FIXME this should also go in GConf
	if (phonemgr_utils_is_bluetooth (device) != FALSE) {
		g_hash_table_insert (driver_device,
				g_strdup (device),
				driver);
	}

	return driver;
}

static char *
phonemgr_utils_driver_for_device (const char *device)
{
	char *driver;

	if (phonemgr_utils_is_bluetooth (device) == FALSE)
		return NULL;

	driver = g_hash_table_lookup (driver_device, device);

	return driver;
}

static void
phonemgr_utils_init_hash_tables (void)
{
	if (driver_device != NULL && driver_model != NULL)
		return;

	driver_device = g_hash_table_new (g_str_hash, g_str_equal);
	driver_model = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (driver_model, "Nokia 6310i", "6310i");
	g_hash_table_insert (driver_model, "Nokia 6230i", "6230");
	g_hash_table_insert (driver_model, "Nokia 6021", "6021");
	g_hash_table_insert (driver_model, "Nokia 6230", "6230");
}

#define MODEL_SIZE 64
char *
phonemgr_utils_guess_driver (char *device)
{
	char *config, **lines, model[MODEL_SIZE];
	gn_data data;
	struct gn_statemachine state;
	char *driver;
	gn_error err;

	memset (model, 0, MODEL_SIZE);
	phonemgr_utils_init_hash_tables ();

	driver = phonemgr_utils_driver_for_device (device);
	if (driver != NULL)
		return driver;

	config = phonemgr_utils_write_config ("AT", device);
	lines = g_strsplit (config, "\n", -1);

	g_free (config);

	if (gn_cfg_memory_read ((const char **)lines) < 0) {
		g_warning ("gn_cfg_memory_read");
		g_strfreev (lines);
		goto bail;
	}
	g_strfreev (lines);

	phonemgr_utils_gn_statemachine_clear (&state);

	if (gn_cfg_phone_load("", &state) < 0) {
		g_warning ("gn_cfg_phone_load");
		goto bail;
	}

	gn_data_clear(&data);
	data.model = model;

	err = gn_gsm_initialise(&state);
	if (err != GN_ERR_NONE) {
		g_warning ("gn_gsm_initialise: %s",
				phonemgr_utils_gn_error_to_string (err));
		goto bail;
	}

	err = gn_sm_functions(GN_OP_Identify, &data, &state);
	if (err != GN_ERR_NONE) {
		g_warning ("gn_sm_functions: %s",
				phonemgr_utils_gn_error_to_string (err));
		goto bail;
	}

	gn_sm_functions(GN_OP_Terminate, NULL, &state);

bail:
	driver = phonemgr_utils_driver_for_model (model, device);

	return driver;
}

