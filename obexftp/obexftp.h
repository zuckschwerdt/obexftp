/**
	\file obexftp/obexftp.h
	Data structures and general functions for OBEX clients and servers.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002-2007 Christian W. Zuckschwerdt <zany@triq.net>

	ObexFTP is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with ObexFTP. If not, see <http://www.gnu.org/>.
 */

#ifndef OBEXFTP_H
#define OBEXFTP_H

#ifdef __cplusplus
extern "C" {
#endif


/** ObexFTP message callback prototype. */
typedef void (*obexftp_info_cb_t) (int event, const char *buf, int len, void *data);

/** ObexFTP message callback events */
enum {
	OBEXFTP_EV_ERRMSG, /* not used / internal error */

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

/** Number of bytes passed at one time to OBEX. */
#define STREAM_CHUNK 4096

/* bt svclass */
#define OBEX_SYNC_SERVICE	0x1104
#define OBEX_PUSH_SERVICE	0x1105
#define OBEX_FTP_SERVICE	0x1106

/* server and client helpers for bt */

char **obexftp_discover(int transport);
char **obexftp_discover_bt_src(const char *src); /* HCI no. or address */
#define	obexftp_discover_bt() \
	obexftp_discover_bt_src(NULL)

char *obexftp_bt_name_src(const char *addr, const char *src);
#define	obexftp_bt_name(addr) \
	obexftp_bt_name_src(addr, NULL)

int obexftp_browse_bt_src(const char *src, const char *addr, int svclass);
#define	obexftp_browse_bt(device, service) \
	obexftp_browse_bt_src(NULL, device, service)
#define	obexftp_browse_bt_ftp(device) \
	obexftp_browse_bt_src(NULL, device, OBEX_FTP_SERVICE)
#define	obexftp_browse_bt_push(device) \
	obexftp_browse_bt_src(NULL, device, OBEX_PUSH_SERVICE)
#define	obexftp_browse_bt_sync(device) \
	obexftp_browse_bt_src(NULL, device, OBEX_SYNC_SERVICE)

int obexftp_sdp_register(int svclass, int channel);
#define	obexftp_sdp_register_ftp(channel) \
	obexftp_sdp_register(OBEX_FTP_SERVICE, channel)
#define	obexftp_sdp_register_push(channel) \
	obexftp_sdp_register(OBEX_PUSH_SERVICE, channel)
#define	obexftp_sdp_register_sync(channel) \
	obexftp_sdp_register(OBEX_SYNC_SERVICE, channel)
int obexftp_sdp_unregister(int svclass);
#define	obexftp_sdp_unregister_ftp() \
	obexftp_sdp_unregister(OBEX_FTP_SERVICE)
#define	obexftp_sdp_unregister_push() \
	obexftp_sdp_unregister(OBEX_PUSH_SERVICE)
#define	obexftp_sdp_unregister_sync() \
	obexftp_sdp_unregister(OBEX_SYNC_SERVICE)


#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP */
