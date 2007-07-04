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

const char *
phonemgr_utils_gn_error_to_string (gn_error error, PhoneMgrError *perr)
{
	g_return_val_if_fail (perr != NULL, NULL);
	*perr = -1;

	switch (error) {
	/* General codes */
	case GN_ERR_NONE:
		return NULL;
	case GN_ERR_FAILED:
		*perr = PHONEMGR_ERROR_COMMAND_FAILED;
		return "Command failed";
	case GN_ERR_UNKNOWNMODEL:
		*perr = PHONEMGR_ERROR_UNKNOWN_MODEL;
		return "Model specified isn't known/supported.";
	case GN_ERR_INVALIDSECURITYCODE:
		return "Invalid Security code.";
	case GN_ERR_INTERNALERROR:
		*perr = PHONEMGR_ERROR_INTERNAL_ERROR;
		return "Problem occured internal to model specific code.";
	case GN_ERR_NOTIMPLEMENTED:
		*perr = PHONEMGR_ERROR_NOT_IMPLEMENTED;
		return "Command called isn't implemented in model.";
	case GN_ERR_NOTSUPPORTED:
		*perr = PHONEMGR_ERROR_NOT_SUPPORTED;
		return "Function not supported by the phone";
	case GN_ERR_USERCANCELED:
		return "User aborted the action.";
	case GN_ERR_UNKNOWN:
		return "Unknown error - well better than nothing";
	case GN_ERR_MEMORYFULL:
		return "The specified memory is full.";
	/* Statemachine */
	case GN_ERR_NOLINK:
		*perr = PHONEMGR_ERROR_NO_LINK;
		return "Couldn't establish link with phone.";
	case GN_ERR_TIMEOUT:
		*perr = PHONEMGR_ERROR_TIME_OUT;
		return "Command timed out.";
	case GN_ERR_TRYAGAIN:
		return "Try again.";
	case GN_ERR_WAITING:
		return "Waiting for the next part of the message.";
	case GN_ERR_NOTREADY:
		*perr = PHONEMGR_ERROR_NOT_READY;
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
		driver = g_strdup (PHONEMGR_DEFAULT_DRIVER);
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
phonemgr_utils_driver_parse_start_tag (GMarkupParseContext *ctx,
				       const char          *element_name,
				       const char         **attr_names,
				       const char         **attr_values,
				       gpointer             data,
				       GError             **error)
{
	const char *phone_name, *phone_driver;

	if (!g_str_equal (element_name, "phone_entry")
			|| attr_names == NULL
			|| attr_values == NULL)
		return;

	phone_name = NULL;
	phone_driver = NULL;

	while (*attr_names && *attr_values)
	{
		if (g_str_equal (*attr_names, "identifier"))
		{
			/* skip if empty */
			if (**attr_values)
				phone_name = *attr_values;
		} else if (g_str_equal (*attr_names, "driver")) {
			/* skip if empty */
			if (**attr_values)
				phone_driver = *attr_values;
		}

		++attr_names;
		++attr_values;
	}

	if (phone_driver == NULL || phone_name == NULL)
		return;

	g_hash_table_insert (driver_model,
			g_strdup (phone_name),
			g_strdup (phone_driver));
}

static void
totem_driver_model_free (void)
{
	g_hash_table_destroy (driver_model);
	driver_model = NULL;
}

static void
phonemgr_utils_start_parse (void)
{
	GError *err = NULL;
	char *buf;
	gsize buf_len;

	driver_model = g_hash_table_new_full
		(g_str_hash, g_str_equal, g_free, g_free);

	g_atexit (totem_driver_model_free);

	if (g_file_get_contents (DATA_DIR"/phones.xml",
				&buf, &buf_len, &err))
	{
		GMarkupParseContext *ctx;
		GMarkupParser parser = { phonemgr_utils_driver_parse_start_tag, NULL, NULL, NULL, NULL };

		ctx = g_markup_parse_context_new (&parser, 0, NULL, NULL);

		if (!g_markup_parse_context_parse (ctx, buf, buf_len, &err))
		{
			g_warning ("Failed to parse '%s': %s\n",
					DATA_DIR"/phones.xml",
					err->message);
			g_error_free (err);
		}

		g_markup_parse_context_free (ctx);
		g_free (buf);
	} else {
		g_warning ("Failed to load '%s': %s\n",
				DATA_DIR"/phones.xml", err->message);
		g_error_free (err);
	}
}

static void
phonemgr_utils_init_hash_tables (void)
{
	if (driver_device != NULL && driver_model != NULL)
		return;

	driver_device = g_hash_table_new (g_str_hash, g_str_equal);
	driver_model = g_hash_table_new (g_str_hash, g_str_equal);

	phonemgr_utils_start_parse ();
}

char *
phonemgr_utils_guess_driver (PhonemgrState *phone_state, const char *device,
			     GError **error)
{
	const char *model;
	char *driver;
	gn_error err;

	phonemgr_utils_init_hash_tables ();

	driver = phonemgr_utils_driver_for_device (device);
	if (driver != NULL)
		return driver;

	if (phone_state == NULL)
		return NULL;

	model = gn_lib_get_phone_model (&phone_state->state);

	if (model == NULL) {
		PhoneMgrError perr;
		g_warning ("gn_lib_get_phone_model failed");
		goto bail;
	}

bail:
	if (model[0] == '\0')
		return NULL;

	driver = phonemgr_utils_driver_for_model (model, device);

	return driver;
}

PhonemgrState *
phonemgr_utils_connect (const char *device, const char *driver, GError **error)
{
	PhonemgrState *phonemgr_state = NULL;
	char *config, **lines;
	gn_data data;
	struct gn_statemachine state;
	gn_error err;

	config = phonemgr_utils_write_config (driver ? driver : PHONEMGR_DEFAULT_DRIVER, device);
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

	err = gn_gsm_initialise(&state);
	if (err != GN_ERR_NONE) {
		PhoneMgrError perr;
		g_warning ("gn_gsm_initialise: %s",
				phonemgr_utils_gn_error_to_string (err, &perr));
		goto bail;
	}

	phonemgr_state = g_new (PhonemgrState, 1);
	phonemgr_state->data = data;
	phonemgr_state->state = state;

bail:

	return phonemgr_state;
}

void
phonemgr_utils_disconnect (PhonemgrState *state)
{
	g_return_if_fail (state != NULL);

	gn_sm_functions (GN_OP_Terminate, NULL, &state->state);
	phonemgr_utils_gn_statemachine_clear (&state->state);
	gn_data_clear (&state->data);
}

void
phonemgr_utils_free (PhonemgrState *state)
{
	g_return_if_fail (state != NULL);
	g_free (state);
}

void
phonemgr_utils_tell_driver (const char *addr)
{
	GError *error = NULL;
	PhonemgrState *phone_state;
	const char *model;

	phone_state = phonemgr_utils_connect (addr, NULL, &error);
	if (phone_state == NULL) {
		g_warning ("Couldn't connect to the '%s' phone: %s", addr, PHONEMGR_CONDERR_STR(error));
		if (error != NULL)
			g_error_free (error);
		return;
	}

	model = gn_lib_get_phone_model (&phone_state->state);

	g_print ("model: '%s'\n", model);

	phonemgr_utils_disconnect (phone_state);
	phonemgr_utils_free (phone_state);
}

