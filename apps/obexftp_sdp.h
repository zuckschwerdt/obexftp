/**
	\file apps/obexftp_sdp.h
	SDP protocol wrappers to register/unregister OBEX servers.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2006 Alan Zhang <vibra@tom.com>

	ObexFTP is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with ObexFTP. If not, see <http://www.gnu.org/>.
 */
 
#ifndef OBEXFTP_SDP_H
#define OBEXFTP_SDP_H


#ifdef __cplusplus
extern "C" {
#endif

void obexftp_sdp_unregister(void); 

int obexftp_sdp_register(int channel);

//int obexftp_sdp_search(bdaddr_t *src, bdaddr_t *dst, uint16_t service);

#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_SDP_H */

