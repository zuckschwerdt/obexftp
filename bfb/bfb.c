/*
 *  bfb/bfb.c: BFB transport encapsulation (used for Siemens mobile equipment)
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
 *  v0.1:  Die,  5 Feb 2002 22:46:19 +0100
 *  v0.4:  Don, 25 Jul 2002 03:16:41 +0200
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* htons */
#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#include "crc.h"
#include "bfb.h"
#include <common.h>

/* Provide convenience macros for handling structure
 * fields through their offsets.
 */
#define STRUCT_OFFSET(struct_type, member)    \
	((uint64_t) ((char*) &((struct_type*) 0)->member))
#define STRUCT_MEMBER_P(struct_p, struct_offset)   \
	((void *) ((char*) (struct_p) + (uint64_t) (struct_offset)))
#define STRUCT_MEMBER(member_type, struct_p, struct_offset)   \
	(*(member_type*) STRUCT_MEMBER_P ((struct_p), (struct_offset)))

/* mobile need little-endianess always */
#if BYTE_ORDER == LITTLE_ENDIAN

#define htoms(A)	(A)
#define htoml(A)	(A)
#define mtohs(A)	(A)
#define mtohl(A)	(A)

#elif BYTE_ORDER == BIG_ENDIAN

#define htoms(A)	((((uint16_t)(A) & 0xff00) >> 8) | \
			 (((uint16_t)(A) & 0x00ff) << 8))
#define htoml(A)	((((uint32_t)(A) & 0xff000000) >> 24) | \
			 (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
			 (((uint32_t)(A) & 0x0000ff00) << 8)  | \
			 (((uint32_t)(A) & 0x000000ff) << 24))
#define mtohs     htoms
#define mtohl     htoml

#else

#error "BYTE_ORDER needs to be either BIG_ENDIAN or LITTLE_ENDIAN."

#endif

/* returns the whole buffer folded with xor. */
uint8_t bfb_checksum(uint8_t *data, int len)
{
	int i;
	uint8_t chk = 0;
      
	for (i=0; i < len; i++)
	     chk ^= data[i];

	return chk;
}

/* stuff data frame into serial cable encapsulation */
/* buffer needs to be of at leaset len+7 size */
/* Type 0x01: "prepare" command. */
/* Type 0x02: first transmission in a row. */
/* Type 0x03: continued transmission. */
/* seq needs to be incremented afterwards. */
int bfb_stuff_data(uint8_t *buffer, uint8_t type, uint8_t *data, int len, int seq)
{
        int i;
        union {
                uint16_t value;
                uint8_t bytes[2];
        } fcs;

	// special case: "attention" packet
	if (type == 1) {
		buffer[0] = 0x01;
		buffer[1] = ~buffer[0];
		return 2;
	}

	// error
	if (type != 2 && type != 3) {
		return 0;
	}

	buffer[0] = type;
	buffer[1] = ~buffer[0];
	buffer[2] = seq;

	fcs.value = htons(len);
	buffer[3] = fcs.bytes[0];
	buffer[4] = fcs.bytes[1];

	// copy data
	memcpy(&buffer[5], data, len);

	// gen CRC
        fcs.value = INIT_FCS;

        for (i=2; i < len+5; i++) {
                fcs.value = irda_fcs(fcs.value, buffer[i]);
        }
        
        fcs.value = ~fcs.value;

	// append CRC to packet
	//fcs.value = htons(fcs.value);
	buffer[len+5] = fcs.bytes[0];
	buffer[len+6] = fcs.bytes[1];

	return len+7;
}

/* send a cmd, subcmd packet, add chk (no parameters) */
int bfb_write_subcmd(FD fd, uint8_t type, uint8_t subtype)
{
	uint8_t buffer[2];

	buffer[0] = subtype;
	buffer[1] = bfb_checksum(buffer, 1);

	return bfb_write_packets(fd, type, buffer, 2);
}

/* send a cmd, subcmd packet */
int bfb_write_subcmd0(FD fd, uint8_t type, uint8_t subtype)
{
	return bfb_write_packets(fd, type, &subtype, 1);
}

/* send a cmd, subcmd, data packet */
int bfb_write_subcmd8(FD fd, uint8_t type, uint8_t subtype, uint8_t p1)
{
	uint8_t buffer[2];

	buffer[0] = subtype;
	buffer[1] = p1;

	return bfb_write_packets(fd, type, buffer, 2);
}

/* send a cmd, subcmd packet, add chk (one word parameter) */
int bfb_write_subcmd1(FD fd, uint8_t type, uint8_t subtype, uint16_t p1)
{
	uint8_t buffer[4];

	buffer[0] = subtype;

	p1 = htoms(p1);	 /* mobile need little-endianess always */
	buffer[1] = STRUCT_MEMBER(uint8_t, &p1, 0);
	buffer[2] = STRUCT_MEMBER(uint8_t, &p1, 1);

	buffer[3] = bfb_checksum(buffer, 3);

	DEBUG(3, "buf: %x %x %x %x",
	      buffer[0], buffer[1], buffer[2], buffer[3]);
	return bfb_write_packets(fd, type, buffer, 4);
}

/* send a cmd, subcmd packet, add chk (two word parameter) */
int bfb_write_subcmd2(FD fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2)
{
	uint8_t buffer[6];

	buffer[0] = subtype;

	p1 = htoms(p1);	 /* mobile need little-endianess always */
	buffer[1] = STRUCT_MEMBER(uint8_t, &p1, 0);
	buffer[2] = STRUCT_MEMBER(uint8_t, &p1, 1);
	p2 = htoms(p2);	 /* mobile need little-endianess always */
	buffer[3] = STRUCT_MEMBER(uint8_t, &p2, 0);
	buffer[4] = STRUCT_MEMBER(uint8_t, &p2, 1);

	buffer[5] = bfb_checksum(buffer, 5);

	DEBUG(3, "buf: %x %x %x %x %x %x",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

	return bfb_write_packets(fd, type, buffer, 6);
}

/* send a cmd, subcmd packet, add chk (three word parameter) */
int bfb_write_subcmd3(FD fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2, uint16_t p3)
{
	uint8_t buffer[8];

	buffer[0] = subtype;

	p1 = htoms(p1);	 /* mobile need little-endianess always */
	buffer[1] = STRUCT_MEMBER(uint8_t, &p1, 0);
	buffer[2] = STRUCT_MEMBER(uint8_t, &p1, 1);
	p2 = htoms(p2);	 /* mobile need little-endianess always */
	buffer[3] = STRUCT_MEMBER(uint8_t, &p2, 0);
	buffer[4] = STRUCT_MEMBER(uint8_t, &p2, 1);
	p3 = htoms(p3);	 /* mobile need little-endianess always */
	buffer[5] = STRUCT_MEMBER(uint8_t, &p3, 0);
	buffer[6] = STRUCT_MEMBER(uint8_t, &p3, 1);

	buffer[7] = bfb_checksum(buffer, 7);

	DEBUG(3, "buf: %x %x  %x %x  %x %x  %x %x",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

	return bfb_write_packets(fd, type, buffer, 8);
}

/* send a cmd, subcmd packet, add long, word parameter */
int bfb_write_subcmd_lw(FD fd, uint8_t type, uint8_t subtype, uint32_t p1, uint16_t p2)
{
	uint8_t buffer[8];

	buffer[0] = subtype;

	p1 = htoml(p1);	 /* mobile need little-endianess always */
	buffer[1] = STRUCT_MEMBER(uint8_t, &p1, 0);
	buffer[2] = STRUCT_MEMBER(uint8_t, &p1, 1);
	buffer[3] = STRUCT_MEMBER(uint8_t, &p1, 2);
	buffer[4] = STRUCT_MEMBER(uint8_t, &p1, 3);
	p2 = htoms(p2);	 /* mobile need little-endianess always */
	buffer[5] = STRUCT_MEMBER(uint8_t, &p2, 0);
	buffer[6] = STRUCT_MEMBER(uint8_t, &p2, 1);

	buffer[7] = bfb_checksum(buffer, 7);

	DEBUG(3, "buf: %02x  %02x %02x %02x %02x  %02x %02x",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

	return bfb_write_packets(fd, type, buffer, 7); /* no chk? */
}


/* send actual packets */
int bfb_write_packets(FD fd, uint8_t type, uint8_t *buffer, int length)
{
	bfb_frame_t *frame;
	int i;
	int l;
#ifdef _WIN32
	DWORD actual;
#else
	int actual;
#endif
	
	// alloc frame buffer
	frame = malloc((length > MAX_PACKET_DATA ? MAX_PACKET_DATA : length) + sizeof (bfb_frame_t));

	for(i=0; i <length; i += MAX_PACKET_DATA) {

		l = length - i;
		if (l > MAX_PACKET_DATA)
			l = MAX_PACKET_DATA;

		frame->type = type;
		frame->len = l;
		frame->chk = frame->type ^ frame->len;

		memcpy(frame->payload, &buffer[i], l);

		/* actual = bfb_io_write(fd, frame, l + sizeof (bfb_frame_t)); */
#ifdef _WIN32
		if(!WriteFile(fd, frame, l + sizeof (bfb_frame_t), &actual, NULL))
			DEBUG(2, __FUNCTION__ "() Write error: %ld", actual);
		DEBUG(3, __FUNCTION__ "() Wrote %ld bytes (expected %d)", actual, l + sizeof (bfb_frame_t));
#else
		actual = write(fd, frame, l + sizeof (bfb_frame_t));
		DEBUG(3, __FUNCTION__ "() Wrote %d bytes (expected %d)", actual, l + sizeof (bfb_frame_t));
#endif


		if (actual < 0 || actual < l + sizeof (bfb_frame_t)) {
			DEBUG(1, __FUNCTION__ "() Write failed");
			free(frame);
			return -1;
		}

	}
	free(frame);
	return i / MAX_PACKET_DATA;
}

int bfb_send_data(FD fd, uint8_t type, uint8_t *data, int length, int seq)
{
	uint8_t *buffer;
	int actual;

	buffer = malloc(length + 7);

	actual = bfb_stuff_data(buffer, type, data, length, seq);
	DEBUG(3, __FUNCTION__ "() Stuffed %d bytes", actual);

	actual = bfb_write_packets(fd, BFB_FRAME_DATA, buffer, actual);
	DEBUG(3, __FUNCTION__ "() Wrote %d packets", actual);

	free(buffer);

	return actual;
}


/* retrieve actual packets */
bfb_frame_t *bfb_read_packets(uint8_t *buffer, int *length)
{
	bfb_frame_t *frame;
	int l;

	DEBUG(3, __FUNCTION__ "() ");

	if (*length < 0) {
		DEBUG(1, __FUNCTION__ "() Wrong length?");
		return NULL;
	}

	if (*length == 0) {
		DEBUG(1, __FUNCTION__ "() No packet?");
		return NULL;
	}

	if (*length < sizeof(bfb_frame_t)) {
		DEBUG(1, __FUNCTION__ "() Short packet?");
		return NULL;
	}
	
	// temp frame
	frame = (bfb_frame_t *)buffer;
	if ((frame->type ^ frame->len) != frame->chk) {
		DEBUG(1, __FUNCTION__ "() Header error?");
		return NULL;
	}

	if (*length < frame->len + sizeof(bfb_frame_t)) {
		DEBUG(1, __FUNCTION__ "() Need more data?");
		return NULL;
	}

	// copy frame from buffer
	l = sizeof(bfb_frame_t) + frame->len;
	frame = malloc(l);
	memcpy(frame, buffer, l);

	// remove frame from buffer
	*length -= l;
	memmove(buffer, &buffer[l], *length);
	
	DEBUG(3, __FUNCTION__ "() Packet %x (%d bytes)", frame->type, frame->len);
	return frame;
}

bfb_data_t *bfb_assemble_data(bfb_data_t *data, int *fraglen, bfb_frame_t *frame)
{
	bfb_data_t *ret;
	int l;

	DEBUG(3, __FUNCTION__ "() ");

	if (frame->type != BFB_FRAME_DATA) {
		DEBUG(1, __FUNCTION__ "() Wrong frame type (%x)?", frame->type);
		return data;
	}

	// temp data
	ret = (bfb_data_t *)frame->payload;
	if (ret->cmd == BFB_DATA_ACK) {
		DEBUG(3, __FUNCTION__ "() Skipping ack");
		return data;
	}
	/*
	if ((ret->cmd != BFB_DATA_FIRST) && (ret->cmd != BFB_DATA_NEXT)) {
		DEBUG(1, __FUNCTION__ "() Wrong data type (%x)?", ret->cmd);
		return data;
	}
	*/

	// copy frame from buffer
	DEBUG(3, __FUNCTION__ "() data: %d, frame: %d", *fraglen, frame->len);
	l = *fraglen + frame->len;
	ret = realloc(data, l);
	//memcpy(ret, data, *fraglen);
	memcpy(&((uint8_t *)ret)[*fraglen], frame->payload, frame->len);

	//free(data);
	*fraglen = l;
	return ret;
}

int bfb_check_data(bfb_data_t *data, int fraglen)
{
        union {
                uint16_t value;
                uint8_t bytes[2];
        } l;

	DEBUG(3, __FUNCTION__ "() ");

	if (data == NULL)
		return -1;

	l.bytes[0] = data->len0;
	l.bytes[1] = data->len1;
	l.value = htons(l.value);

	DEBUG(3, __FUNCTION__ "() fragment size is %d", fraglen);
	DEBUG(3, __FUNCTION__ "() expected len %d", l.value);
	DEBUG(3, __FUNCTION__ "() data size is %d", fraglen-sizeof(bfb_data_t));

	if (fraglen-sizeof(bfb_data_t) + 2 < l.value)
		return 0;

/*
	// check CRC
	if (sizeof(data) < l.value)
		return -1;
*/

	DEBUG(2, __FUNCTION__ "() data ready!");
	return 1;

}
