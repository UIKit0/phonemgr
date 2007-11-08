/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  PhoneListener utilities
 *
 *  Copyright (C) 2005-2007 Bastien Nocera <hadess@hadess.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __PHONEMGR_UTILS_H__
#define __PHONEMGR_UTILS_H__

#include <glib-object.h>
#include <gnokii.h>

#include "phonemgr-listener.h"

G_BEGIN_DECLS

#define PHONEMGR_DEFAULT_DRIVER "AT"
#define PHONEMGR_DEFAULT_USB_DRIVER "6510"
#define PHONEMGR_CONDERR_STR(err) (err ? err->message : "No reason")

typedef enum {
	PHONEMGR_CONNECTION_BLUETOOTH,
	PHONEMGR_CONNECTION_SERIAL,
	PHONEMGR_CONNECTION_IRDA,
	PHONEMGR_CONNECTION_USB
} PhonemgrConnectionType;

typedef struct PhonemgrState PhonemgrState;

struct PhonemgrState {
	gn_data data;
	struct gn_statemachine state;
};

PhonemgrConnectionType phonemgr_utils_address_is (const char *addr);
int phonemgr_utils_get_channel (const char *device);
char *phonemgr_utils_write_config (const char *driver,
				   const char *addr,
				   int channel);
char *phonemgr_utils_guess_driver (PhonemgrState *phone_state,
				   const char *device,
				   GError **error);
void phonemgr_utils_gn_statemachine_clear (struct gn_statemachine *state);
const char *phonemgr_utils_gn_error_to_string (gn_error error,
					       PhoneMgrError *perr);

PhonemgrState *phonemgr_utils_connect (const char *device,
				       const char *driver,
				       int channel,
				       gboolean debug,
				       GError **error);
void phonemgr_utils_disconnect (PhonemgrState *phone_state);
void phonemgr_utils_free (PhonemgrState *phone_state);
void phonemgr_utils_tell_driver (const char *addr);
void phonemgr_utils_write_gnokii_config (const char *addr);
char *phonemgr_utils_config_append_debug (const char *config);
gboolean phonemgr_utils_connection_is_supported (PhonemgrConnectionType type);
time_t gn_timestamp_to_gtime (gn_timestamp stamp);

G_END_DECLS

#endif
