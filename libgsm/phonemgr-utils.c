/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * PhoneManager Utilities
 * Copyright (C) 2003-2004 Edd Dumbill <edd@usefulinc.com>
 * Copyright (C) 2005-2007 Bastien Nocera <hadess@hadess.net>
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
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gnokii.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "phonemgr-utils.h"

/* This is the hash table containing information about driver <-> device
 * match */
static GHashTable *driver_device = NULL;
/* This is the hash table containing information about driver <-> model
 * match */
static GHashTable *driver_model = NULL;

static void phonemgr_utils_init_hash_tables (void);

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

PhonemgrConnectionType
phonemgr_utils_address_is (const char *addr)
{
	struct stat buf;

	if (g_stat (addr, &buf) > 0 && S_ISCHR (buf.st_mode)) {
		//FIXME this could also be IrDA
		return PHONEMGR_CONNECTION_SERIAL;
	} else if (strlen (addr) == 17) {
		//FIXME find a better match
		return PHONEMGR_CONNECTION_BLUETOOTH;
	} else {
		//FIXME find a better match
		return PHONEMGR_CONNECTION_USB;
	}
}

static int
get_rfcomm_channel (sdp_record_t *rec)
{
	int channel = -1;
	sdp_list_t *protos = NULL;
	sdp_data_t *data;
	char *name = NULL;

	if (sdp_get_access_protos (rec, &protos) != 0)
		goto end;

	data = sdp_data_get(rec, SDP_ATTR_SVCNAME_PRIMARY);
	if (data)
		name = g_strdup_printf ("%.*s", data->unitSize, data->val.str);

	/* We can't seem to connect to the PC Suite channel */
	if (strstr (name, "Nokia PC Suite") != NULL)
		goto end;
	/* And that type of channel on Nokia Symbian phones doesn't
	 * work either */
	if (strstr (name, "Bluetooth Serial Port") != NULL)
		goto end;
	/* Avoid the m-Router channel, same as the PC Suite on Sony Ericsson phones */
	if (strstr (name, "m-Router Connectivity") != NULL)
		goto end;

	channel = sdp_get_proto_port (protos, RFCOMM_UUID);
end:
	g_free (name);
	sdp_list_foreach(protos, (sdp_list_func_t)sdp_list_free, 0);
	sdp_list_free(protos, 0);
	return channel;
}

/* Determine whether the given device supports Serial or Dial-Up Networking, and if so
 * what the RFCOMM channel number for the service is.
 */
static int
find_service_channel (bdaddr_t *adapter, bdaddr_t *device, uint16_t svclass_id)
{
	sdp_session_t *sdp = NULL;
	sdp_list_t *search = NULL, *attrs = NULL, *recs = NULL, *tmp;
	uuid_t browse_uuid, service_id, obex_uuid;
	uint32_t range = 0x0000ffff;
	int channel = -1;

	sdp = sdp_connect (adapter, device, SDP_RETRY_IF_BUSY);
	if (!sdp)
		goto end;

	sdp_uuid16_create(&browse_uuid, PUBLIC_BROWSE_GROUP);
	sdp_uuid16_create(&service_id, svclass_id);
	search = sdp_list_append (NULL, &browse_uuid);
	search = sdp_list_append (search, &service_id);

	attrs = sdp_list_append (NULL, &range);

	if (sdp_service_search_attr_req (sdp, search,
					 SDP_ATTR_REQ_RANGE, attrs,
					 &recs))
		goto end;

	for (tmp = recs; tmp != NULL; tmp = tmp->next) {
		sdp_record_t *rec = tmp->data;

		/* If this service is better than what we've
		 * previously seen, try and get the channel number.
		 */
		channel = get_rfcomm_channel (rec);
		if (channel > 0)
			goto end;
	}

end:
	sdp_list_free (recs, (sdp_free_func_t)sdp_record_free);
	sdp_list_free (search, NULL);
	sdp_list_free (attrs, NULL);
	sdp_close(sdp);

	return channel;
}

int
phonemgr_utils_get_channel (const char *device)
{
	bdaddr_t src, dst;
	int channel;

	bacpy (&src, BDADDR_ANY);
	str2ba(device, &dst);

	channel = find_service_channel (&src, &dst, SERIAL_PORT_SVCLASS_ID);
	if (channel < 0)
		channel = find_service_channel (&src, &dst, DIALUP_NET_SVCLASS_ID);

	return channel;
}

char *
phonemgr_utils_write_config (const char *driver, const char *addr, int channel)
{
	PhonemgrConnectionType type;

	type = phonemgr_utils_address_is (addr);

	if (type == PHONEMGR_CONNECTION_BLUETOOTH) {
		if (channel > 0) {
			return g_strdup_printf ("[global]\n"
						"port = %s\n"
						"model = %s\n"
						"connection = bluetooth\n"
						"rfcomm_channel = %d\n",
						addr,
						driver,
						channel);
		} else {
			return g_strdup_printf ("[global]\n"
						"port = %s\n"
						"model = %s\n"
						"connection = bluetooth\n", addr, driver);
		}
	} else if (type == PHONEMGR_CONNECTION_USB) {
		return g_strdup_printf ("[global]\n"
					"port = %s\n"
					"model = %s\n"
					"connection = dku2libusb\n", addr, driver);
	} else {
		return g_strdup_printf ("[global]\n"
			"port = %s\n"
			"model = %s\n"
			"connection = serial\n", addr, driver);
	}
}

char *
phonemgr_utils_config_append_debug (const char *config)
{
	return g_strdup_printf ("%s\n"
				"[logging]\n"
				"debug = on\n",
				config);
}

static char *
phonemgr_utils_driver_for_model (const char *model, const char *device)
{
	char *driver;

	if (driver_model == NULL)
		phonemgr_utils_init_hash_tables ();

	driver = g_hash_table_lookup (driver_model, model);
	if (driver == NULL) {
		g_message ("Model %s not supported natively", model);
		driver = g_strdup (PHONEMGR_DEFAULT_DRIVER);
	} else {
		driver = g_strdup (driver);
	}

	/* Add it to the list if it's a bluetooth device */
	//FIXME this should also go in GConf
	if (phonemgr_utils_address_is (device) == PHONEMGR_CONNECTION_BLUETOOTH) {
		g_hash_table_insert (driver_device,
				g_strdup (device),
				driver);
	}

	return driver;
}

static char *
phonemgr_utils_driver_for_device (const char *device)
{
	PhonemgrConnectionType type;
	char *driver;

	type = phonemgr_utils_address_is (device);
	if (type == PHONEMGR_CONNECTION_USB)
		return PHONEMGR_DEFAULT_USB_DRIVER;
	if (type != PHONEMGR_CONNECTION_BLUETOOTH)
		return NULL;

	if (driver_device == NULL)
		phonemgr_utils_init_hash_tables ();

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
phonemgr_utils_connect (const char *device, const char *driver, int channel, gboolean debug, GError **error)
{
	PhonemgrState *phone_state = NULL;
	PhonemgrConnectionType type;
	char *config, **lines;
	const char *default_driver;
	gn_data data;
	struct gn_statemachine state;
	gn_error err;

	type = phonemgr_utils_address_is (device);
	if (phonemgr_utils_connection_is_supported (type) == FALSE) {
		//FIXME set the error message
		return NULL;
	}
	if (type == PHONEMGR_CONNECTION_USB)
		default_driver = PHONEMGR_DEFAULT_USB_DRIVER;
	else
		default_driver = PHONEMGR_DEFAULT_DRIVER;

	config = phonemgr_utils_write_config (driver ? driver : default_driver, device, channel);
	if (debug != FALSE) {
		char *debug;

		debug = phonemgr_utils_config_append_debug (config);
		g_free (config);
		lines = g_strsplit (debug, "\n", -1);
		g_free (debug);
	} else {
		lines = g_strsplit (config, "\n", -1);
		g_free (config);
	}

	if (gn_cfg_memory_read ((const char **)lines) < 0) {
		g_warning ("gn_cfg_memory_read");
		g_strfreev (lines);
		return NULL;
	}
	g_strfreev (lines);

	phonemgr_utils_gn_statemachine_clear (&state);

	if (gn_cfg_phone_load("", &state) < 0) {
		g_warning ("gn_cfg_phone_load");
		return NULL;
	}

	err = gn_gsm_initialise(&state);
	if (err != GN_ERR_NONE) {
		PhoneMgrError perr;
		g_warning ("gn_gsm_initialise: %s",
			   phonemgr_utils_gn_error_to_string (err, &perr));
		return NULL;
	}

	phone_state = g_new (PhonemgrState, 1);
	phone_state->data = data;
	phone_state->state = state;

	return phone_state;
}

void
phonemgr_utils_disconnect (PhonemgrState *phone_state)
{
	g_return_if_fail (phone_state != NULL);

	gn_lib_phone_close (&phone_state->state);
	phonemgr_utils_gn_statemachine_clear (&phone_state->state);
	gn_data_clear (&phone_state->data);
}

void
phonemgr_utils_free (PhonemgrState *phone_state)
{
	g_return_if_fail (phone_state != NULL);
	g_free (phone_state);
}

void
phonemgr_utils_tell_driver (const char *addr)
{
	GError *error = NULL;
	PhonemgrState *phone_state;
	const char *model;
	char *driver;
	int channel;

	channel = phonemgr_utils_get_channel (addr);
	phone_state = phonemgr_utils_connect (addr, NULL, channel, FALSE, &error);
	if (phone_state == NULL) {
		g_warning ("Couldn't connect to the '%s' phone: %s", addr, PHONEMGR_CONDERR_STR(error));
		if (error != NULL)
			g_error_free (error);
		return;
	}

	model = gn_lib_get_phone_model (&phone_state->state);

	g_print ("model: '%s'\n", model);
	driver = phonemgr_utils_driver_for_model (model, addr);
	g_print ("guessed driver: '%s'\n", driver);
	g_free (driver);

	phonemgr_utils_disconnect (phone_state);
	phonemgr_utils_free (phone_state);
}

void
phonemgr_utils_write_gnokii_config (const char *addr)
{
	GError *error = NULL;
	PhonemgrState *phone_state;
	char *driver, *config, *debug;
	int channel;

	channel = phonemgr_utils_get_channel (addr);
	phone_state = phonemgr_utils_connect (addr, NULL, channel, FALSE, &error);
	if (phone_state == NULL) {
		g_warning ("Couldn't connect to the '%s' phone: %s", addr, PHONEMGR_CONDERR_STR(error));
		if (error != NULL)
			g_error_free (error);
		return;
	}

	driver = phonemgr_utils_guess_driver (phone_state, addr, NULL);

	config = phonemgr_utils_write_config (driver, addr, channel);
	g_free (driver);
	debug = phonemgr_utils_config_append_debug (config);
	g_free (config);
	if (g_file_set_contents ("gnokiirc", debug, -1, NULL) == FALSE) {
		g_warning ("Couldn't write gnokiirc file in the current directory");
	} else {
		g_print ("Configuration file written in the current directory\n");
		g_print ("Move gnokiirc to ~/.gnokiirc to start debugging with gnokii\n");
	}
	g_free (debug);

	phonemgr_utils_disconnect (phone_state);
	phonemgr_utils_free (phone_state);
}

time_t
gn_timestamp_to_gtime (gn_timestamp stamp)
{
	GDate *date;
	time_t time = 0;
	int i;

	if (gn_timestamp_isvalid (stamp) == FALSE)
		return time;

	date = g_date_new_dmy (stamp.day, stamp.month, stamp.year);
	for (i = 1970; i < stamp.year; i++) {
		if (g_date_is_leap_year (i) != FALSE)
			time += 3600 * 24 * 366;
		else
			time += 3600 * 24 * 365;
	}
	time += g_date_get_day_of_year (date) * 3600 * 24;
	time += stamp.hour * 3600 + stamp.minute * 60 + stamp.second;

	g_free (date);

	return time;
}

gboolean
phonemgr_utils_connection_is_supported (PhonemgrConnectionType type)
{
	gn_connection_type conntype;

	switch (type) {
	case PHONEMGR_CONNECTION_BLUETOOTH:
		conntype = GN_CT_Bluetooth;
		break;
	case PHONEMGR_CONNECTION_SERIAL:
		conntype = GN_CT_Serial;
		break;
	case PHONEMGR_CONNECTION_IRDA:
		conntype = GN_CT_Irda;
		if (!gn_lib_is_connectiontype_supported (conntype))
			conntype = GN_CT_Infrared;
		break;
	case PHONEMGR_CONNECTION_USB:
		conntype = GN_CT_DKU2LIBUSB;
		if (!gn_lib_is_connectiontype_supported (conntype))
			conntype = GN_CT_DKU2;
		break;
	default:
		g_assert_not_reached ();
	}

	return gn_lib_is_connectiontype_supported (conntype);
}

