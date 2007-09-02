/**
	\file bfb/bfb.h
	BFB transport encapsulation (used for Siemens mobile equipment).
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

#ifndef BFB_H
#define BFB_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
typedef HANDLE fd_t;
#else
typedef int fd_t;
#endif

#pragma pack(1)
typedef struct {
	uint8_t type;
	uint8_t len;
        uint8_t chk;
        uint8_t payload[0]; /* ... up to 32 */
	/* uint8_t xor; ? */
} bfb_frame_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	uint8_t cmd;
	uint8_t chk;
	uint8_t seq;
	uint8_t len0;
	uint8_t len1;
	uint8_t data[0]; /* ... up to 518 ? */
	/* uint16_t crc; */
} bfb_data_t;
#pragma pack()


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

int	bfb_write_subcmd(fd_t fd, uint8_t type, uint8_t subtype);
int	bfb_write_subcmd0(fd_t fd, uint8_t type, uint8_t subtype);
int	bfb_write_subcmd8(fd_t fd, uint8_t type, uint8_t subtype, uint8_t p1);
int	bfb_write_subcmd1(fd_t fd, uint8_t type, uint8_t subtype, uint16_t p1);
int	bfb_write_subcmd2(fd_t fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2);
int	bfb_write_subcmd3(fd_t fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2, uint16_t p3);
int	bfb_write_subcmd_lw(fd_t fd, uint8_t type, uint8_t subtype, uint32_t p1, uint16_t p2);

int	bfb_stuff_data(/*@out@*/ uint8_t *buffer, uint8_t type, uint8_t *data, uint16_t len, uint8_t seq);

int	bfb_write_packets(fd_t fd, uint8_t type, uint8_t *buffer, int length);

#define bfb_write_at(fd, data) \
	bfb_write_packets(fd, BFB_FRAME_AT, data, strlen(data))

#define bfb_write_key(fd, data) \
	bfb_write_subcmd8(fd, BFB_FRAME_KEY, BFB_KEY_PRESS, data)

int	bfb_send_data(fd_t fd, uint8_t type, uint8_t *data, uint16_t length, uint8_t seq);

#define bfb_send_ack(fd) \
	bfb_send_data(fd, BFB_DATA_ACK, NULL, 0, 0)

#define bfb_send_first(fd, data, length) \
	bfb_send_data(fd, BFB_DATA_FIRST, data, length, 0)

#define bfb_send_next(fd, data, length, seq) \
	bfb_send_data(fd, BFB_DATA_NEXT, data, length, seq)


/*@null@*/ bfb_frame_t *
	bfb_read_packets(uint8_t *buffer, int *length);

int	bfb_assemble_data(/*@null@*/ /*@out@*/ bfb_data_t **data, int *size, int *len, bfb_frame_t *frame);

int	bfb_check_data(bfb_data_t *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* BFB_H */
