/*
 *  bfb/bfb.h
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

#ifndef BFB_H
#define BFB_H

#include <glib.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE FD;
#else
typedef int FD;
#endif

#define	BFB_LOG_DOMAIN	"bfb"

typedef struct {
	guint8 type;
	guint8 len;
        guint8 chk;
        guint8 payload[0]; //...
	// guint8 xor; ?
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


#define BFB_FRAME_CONNECT 0x02   /* ^B */
#define BFB_FRAME_INTERFACE 0x01 /* ^A */
#define BFB_FRAME_KEY 0x05       /* ^E */
#define BFB_FRAME_AT 0x06        /* ^F */
#define BFB_FRAME_EEPROM 0x14    /* ^N */
#define BFB_FRAME_DATA 0x16      /* ^P */

#define BFB_CONNECT_HELLO 0x14   /* ^N */
#define BFB_CONNECT_HELLO_ACK 0xaa

#define BFB_KEY_PRESS 0x06        /* ^F */

#define MAX_PACKET_DATA 32
#define BFB_DATA_ACK 0x01 /* aka ok */
#define BFB_DATA_FIRST 0x02 /* first transmission in a row */
#define BFB_DATA_NEXT 0x03 /* continued transmission */

guint8	bfb_checksum(guint8 *data, gint len);

gint	bfb_write_subcmd(FD fd, guint8 type, guint8 subtype);
gint	bfb_write_subcmd0(FD fd, guint8 type, guint8 subtype);
gint	bfb_write_subcmd8(FD fd, guint8 type, guint8 subtype, guint8 p1);
gint	bfb_write_subcmd1(FD fd, guint8 type, guint8 subtype, guint16 p1);

/* send a cmd, subcmd packet, add chk (two word parameter) */
gint	bfb_write_subcmd2(FD fd, guint8 type, guint8 subtype, guint16 p1, guint16 p2);

/* send a cmd, subcmd packet, add chk (three word parameter) */
gint	bfb_write_subcmd3(FD fd, guint8 type, guint8 subtype, guint16 p1, guint16 p2, guint16 p3);

/* send a cmd, subcmd packet, add long, word parameter */
gint	bfb_write_subcmd_lw(FD fd, guint8 type, guint8 subtype, guint32 p1, guint16 p2);

gint	bfb_stuff_data(guint8 *buffer, guint8 type, guint8 *data, gint len, gint seq);

gint	bfb_write_packets(FD fd, guint8 type, guint8 *buffer, gint length);

#define bfb_write_at(fd, data) \
	bfb_write_packets(fd, BFB_FRAME_AT, data, strlen(data))

#define bfb_write_key(fd, data) \
	bfb_write_subcmd8(fd, BFB_FRAME_KEY, BFB_KEY_PRESS, data)

gint	bfb_send_data(FD fd, guint8 type, guint8 *data, gint length, gint seq);

#define bfb_send_ack(fd) \
	bfb_send_data(fd, BFB_DATA_ACK, NULL, 0, 0)

#define bfb_send_first(fd, data, length) \
	bfb_send_data(fd, BFB_DATA_FIRST, data, length, 0)

#define bfb_send_next(fd, data, length, seq) \
	bfb_send_data(fd, BFB_DATA_NEXT, data, length, seq)

bfb_frame_t *
	bfb_read_packets(guint8 *buffer, gint *length);

bfb_data_t *
	bfb_assemble_data(bfb_data_t *data, gint *fraglen, bfb_frame_t *frame);

gint	bfb_check_data(bfb_data_t *data, gint fraglen);

#endif /* BFB_H */
