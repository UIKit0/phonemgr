/*
 * main.c - entry point and libpurple boilerplate for telepathy-sms
 * Copyright (C) 2007 Will Thompson
 * Copyright (C) 2007 Collabora Ltd.
 * Portions taken from libpurple/examples/nullclient.c:
 *   Copyright (C) 2007 Sadrul Habib Chowdhury, Sean Egan, Gary Kramlich,
 *                      Mark Doliner, Richard Laager
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include <string.h>
#include <errno.h>
#include <signal.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <telepathy-glib/run.h>
#include <telepathy-glib/debug.h>

//#include "defines.h"
#include "debug.h"
#include "connection-manager.h"

int
main(int argc,
     char **argv)
{
    int ret = 0;

    g_set_prgname("telepathy-sms");

    signal (SIGCHLD, SIG_IGN);

    tp_debug_set_flags_from_env ("SMS_DEBUG");
    ret = tp_run_connection_manager ("telepathy-sms", PACKAGE_VERSION,
    				     sms_connection_manager_get, argc, argv);

    return ret;
}

