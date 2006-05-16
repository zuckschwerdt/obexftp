/*
 *
 *  obexftp/obexftp_sdp.h: Transfer from/to Mobile Equipment via OBEX
 *
 *  Copyright (c) 2006 Alan Zhang <vibra@tom.com>
 *
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
 
#ifndef OBEXFTP_SDP_H
#define OBEXFTP_SDP_H


#ifdef __cplusplus
extern "C" {
#endif

void obexftp_sdp_unregister(void); 

int obexftp_sdp_register(void);

//int obexftp_sdp_search(bdaddr_t *src, bdaddr_t *dst, uint16_t service);

#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_SDP_H */

