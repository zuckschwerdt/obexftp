/*
 *  obexftp/obexftp.h: Transfer from/to Siemens Mobile Equipment via OBEX
 *
 *  Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */
/*
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 */

#ifndef OBEXFTP_H
#define OBEXFTP_H

#ifdef __cplusplus
extern "C" {
#endif

#define	OBEXFTP_LOG_DOMAIN	"obexftp"

typedef void (*obexftp_info_cb_t) (int event, const char *buf, int len, void *data);

enum {
	OBEXFTP_EV_ERRMSG,

	OBEXFTP_EV_OK,
	OBEXFTP_EV_ERR,

	OBEXFTP_EV_CONNECTING,
	OBEXFTP_EV_DISCONNECTING,
	OBEXFTP_EV_SENDING,

	OBEXFTP_EV_LISTENING,
	OBEXFTP_EV_CONNECTIND,
	OBEXFTP_EV_DISCONNECTIND,
	OBEXFTP_EV_RECEIVING,

	OBEXFTP_EV_BODY,
	OBEXFTP_EV_INFO,
	OBEXFTP_EV_PROGRESS, /* approx. every 1KByte */
};

/* Number of bytes passed at one time to OBEX */
#define STREAM_CHUNK 4096

#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP */
