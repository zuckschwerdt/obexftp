/*
 *                
 * Filename:      bfb.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Die,  5 Feb 2002 22:46:19 +0100
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

#include <glib.h>

typedef struct {
        guint8 type;
        guint8 len;
        guint8 chk;
        guint8 payload[0]; //...
} bfb_frame_t;

typedef struct {
	guint8 cmd;
	guint8 chk;
	guint8 seq;
	guint8 len0;
	guint8 len1;
	guint8 data[0]; //...
	// guint16 crc;
} bfb_data_t;


#define BFB_FRAME_CONNECT 0x02
#define BFB_FRAME_INTERFACE 0x01
#define BFB_FRAME_AT 0x06
#define BFB_FRAME_DATA 0x16

#define MAX_PACKET_DATA 32
#define BFB_DATA_ACK 0x01 /* aka ok */
#define BFB_DATA_FIRST 0x02 /* first transmission in a row */
#define BFB_DATA_NEXT 0x03 /* continued transmission */


gint bfb_stuff_data(guint8 *buffer, guint8 type, guint8 *data, gint len, gint seq);

gint bfb_write_packets(int fd, guint8 type, guint8 *buffer, gint length);

gint bfb_send_data(int fd, guint8 type, guint8 *data, gint length, gint seq);

#define bfb_send_ack(fd) \
	bfb_send_data(fd, BFB_DATA_ACK, NULL, 0, 0)

#define bfb_send_first(fd, data, length) \
	bfb_send_data(fd, BFB_DATA_FIRST, data, length, 0)

#define bfb_send_next(fd, data, length, seq) \
	bfb_send_data(fd, BFB_DATA_NEXT, data, length, seq)

bfb_frame_t *bfb_read_packets(guint8 *buffer, gint *length);

bfb_data_t *bfb_assemble_data(bfb_data_t *data, gint *fraglen, bfb_frame_t *frame);

gint bfb_check_data(bfb_data_t *data, gint fraglen);