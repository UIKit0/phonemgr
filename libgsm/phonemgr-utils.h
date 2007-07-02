/*
 *  PhoneListener utilities
 *
 *  Copyright (C) 2005 Bastien Nocera <hadess@hadess.net>
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
#define PHONEMGR_CONDERR_STR(err) (err ? err->message : "No reason")

typedef struct PhonemgrState PhonemgrState;

struct PhonemgrState {
	gn_data data;
	struct gn_statemachine state;
};

char *phonemgr_utils_write_config (const char *driver, const char *addr);
char *phonemgr_utils_guess_driver (PhonemgrState *state,
				   const char *device, GError **error);
void phonemgr_utils_gn_statemachine_clear (struct gn_statemachine *state);
const char *phonemgr_utils_gn_error_to_string (gn_error error,
					       PhoneMgrError *perr);

PhonemgrState *phonemgr_utils_connect (const char *device, const char *driver,
				       GError **error);
void phonemgr_utils_disconnect (PhonemgrState *state);
void phonemgr_utils_free (PhonemgrState *state);
void phonemgr_utils_tell_driver (const char *addr);

G_END_DECLS

#endif
