/*
 *                
 * Filename:      flexmem.h
 * Version:       
 * Description:   Transfer from/to Siemens Mobile Equipment via OBEX
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 * Modified at:   Don,  7 Feb 2002 12:24:55 +0100
 * Modified by:   Christian W. Zuckschwerdt <zany@triq.net>
 *
 *   Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */

#ifndef IRCP_H
#define IRCP_H

#include <glib.h>

typedef void (*ircp_info_cb_t)(gint event, gpointer param);

enum {
	IRCP_EV_ERRMSG,

	IRCP_EV_OK,
	IRCP_EV_ERR,

	IRCP_EV_CONNECTING,
	IRCP_EV_DISCONNECTING,
	IRCP_EV_SENDING,

	IRCP_EV_LISTENING,
	IRCP_EV_CONNECTIND,
	IRCP_EV_DISCONNECTIND,
	IRCP_EV_RECEIVING,

	IRCP_EV_BODY,
	IRCP_EV_INFO,
};

/* Number of bytes passed at one time to OBEX */
#define STREAM_CHUNK 4096

#endif
