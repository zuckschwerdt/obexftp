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

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE FD;
#else
typedef int FD;
#endif

#define	BFB_LOG_DOMAIN	"bfb"

typedef struct {
	uint8_t type;
	uint8_t len;
        uint8_t chk;
        uint8_t payload[0]; //...
	// uint8_t xor; ?
} bfb_frame_t;

typedef struct {
	uint8_t cmd;
	uint8_t chk;
	uint8_t seq;
	uint8_t len0;
	uint8_t len1;
	uint8_t data[0]; //...
	// uint16_t crc;
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

uint8_t	bfb_checksum(uint8_t *data, int len);

int	bfb_write_subcmd(FD fd, uint8_t type, uint8_t subtype);
int	bfb_write_subcmd0(FD fd, uint8_t type, uint8_t subtype);
int	bfb_write_subcmd8(FD fd, uint8_t type, uint8_t subtype, uint8_t p1);
int	bfb_write_subcmd1(FD fd, uint8_t type, uint8_t subtype, uint16_t p1);

/* send a cmd, subcmd packet, add chk (two word parameter) */
int	bfb_write_subcmd2(FD fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2);

/* send a cmd, subcmd packet, add chk (three word parameter) */
int	bfb_write_subcmd3(FD fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2, uint16_t p3);

/* send a cmd, subcmd packet, add long, word parameter */
int	bfb_write_subcmd_lw(FD fd, uint8_t type, uint8_t subtype, uint32_t p1, uint16_t p2);

int	bfb_stuff_data(uint8_t *buffer, uint8_t type, uint8_t *data, int len, int seq);

int	bfb_write_packets(FD fd, uint8_t type, uint8_t *buffer, int length);

#define bfb_write_at(fd, data) \
	bfb_write_packets(fd, BFB_FRAME_AT, data, strlen(data))

#define bfb_write_key(fd, data) \
	bfb_write_subcmd8(fd, BFB_FRAME_KEY, BFB_KEY_PRESS, data)

int	bfb_send_data(FD fd, uint8_t type, uint8_t *data, int length, int seq);

#define bfb_send_ack(fd) \
	bfb_send_data(fd, BFB_DATA_ACK, NULL, 0, 0)

#define bfb_send_first(fd, data, length) \
	bfb_send_data(fd, BFB_DATA_FIRST, data, length, 0)

#define bfb_send_next(fd, data, length, seq) \
	bfb_send_data(fd, BFB_DATA_NEXT, data, length, seq)

/*@null@*/ bfb_frame_t *
	bfb_read_packets(uint8_t *buffer, int *length);

/*@null@*/ bfb_data_t *
	bfb_assemble_data(bfb_data_t *data, int *fraglen, bfb_frame_t *frame);

int	bfb_check_data(bfb_data_t *data, int fraglen);

#endif /* BFB_H */
