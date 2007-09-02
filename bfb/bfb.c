/**
	\file bfb/bfb.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* htons */
#ifdef _WIN32
#include <winsock2.h>
#include <sys/param.h> /* for endianess */
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
/* Solaris: _LITTLE_ENDIAN / _BIG_ENDIAN */
/* Linux: */
#if (BYTE_ORDER == LITTLE_ENDIAN) || defined (_LITTLE_ENDIAN)

#define htoms(A)	(A)
#define htoml(A)	(A)
#define mtohs(A)	(A)
#define mtohl(A)	(A)

#elif (BYTE_ORDER == BIG_ENDIAN) || defined (_BIG_ENDIAN)

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


/**
	Returns the whole buffer folded with xor.
 */
uint8_t bfb_checksum(uint8_t *data, int len)
{
	int i;
	uint8_t chk = 0;
      
	for (i=0; i < len; i++)
	     chk ^= data[i];

	return chk;
}


/**
	Stuff data frame into serial cable encapsulation.
	buffer needs to be of at leaset len+7 size
	Type 0x01: "prepare" command.
	Type 0x02: first transmission in a row.
	Type 0x03: continued transmission.
	seq needs to be incremented afterwards.
 */
int bfb_stuff_data(uint8_t *buffer, uint8_t type, uint8_t *data, uint16_t len, uint8_t seq)
{
        int i;
        union {
                uint16_t value;
                uint8_t bytes[2];
        } fcs;

	/* special case: "attention" packet */
	if (type == 1) {
		buffer[0] = 0x01;
		buffer[1] = ~buffer[0];
		return 2;
	}

	/* error */
	if (type != 2 && type != 3) {
		buffer[0] = 0x00; /* just terminate the buffer */
		return 0;
	}

	buffer[0] = type;
	buffer[1] = ~buffer[0];
	buffer[2] = seq;

	fcs.value = htons(len);
	buffer[3] = fcs.bytes[0];
	buffer[4] = fcs.bytes[1];

	/* copy data */
	memcpy(&buffer[5], data, len);

	/* gen CRC */
        fcs.value = INIT_FCS;

        for (i=2; i < len+5; i++) {
                fcs.value = irda_fcs(fcs.value, buffer[i]);
        }
        
        fcs.value = ~fcs.value;

	/* append CRC to packet */
	/* fcs.value = htons(fcs.value); */
	buffer[len+5] = fcs.bytes[0];
	buffer[len+6] = fcs.bytes[1];

	return len+7;
}


/**
	Send a cmd, subcmd packet, add chk (no parameters).
 */
int bfb_write_subcmd(fd_t fd, uint8_t type, uint8_t subtype)
{
	uint8_t buffer[2];

	buffer[0] = subtype;
	buffer[1] = bfb_checksum(buffer, 1);

	return bfb_write_packets(fd, type, buffer, 2);
}


/**
	Send a cmd, subcmd packet.
 */
int bfb_write_subcmd0(fd_t fd, uint8_t type, uint8_t subtype)
{
	return bfb_write_packets(fd, type, &subtype, 1);
}


/**
	Send a cmd, subcmd, data packet.
 */
int bfb_write_subcmd8(fd_t fd, uint8_t type, uint8_t subtype, uint8_t p1)
{
	uint8_t buffer[2];

	buffer[0] = subtype;
	buffer[1] = p1;

	return bfb_write_packets(fd, type, buffer, 2);
}


/**
	Send a cmd, subcmd packet, add chk (one word parameter).
 */
int bfb_write_subcmd1(fd_t fd, uint8_t type, uint8_t subtype, uint16_t p1)
{
	uint8_t buffer[4];

	buffer[0] = subtype;

	p1 = htoms(p1);	 /* mobile need little-endianess always */
	buffer[1] = STRUCT_MEMBER(uint8_t, &p1, 0);
	buffer[2] = STRUCT_MEMBER(uint8_t, &p1, 1);

	buffer[3] = bfb_checksum(buffer, 3);

	DEBUG(3, "buf: %x %x %x %x\n",
	      buffer[0], buffer[1], buffer[2], buffer[3]);
	return bfb_write_packets(fd, type, buffer, 4);
}


/**
	Send a cmd, subcmd packet, add chk (two word parameter).
 */
int bfb_write_subcmd2(fd_t fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2)
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

	DEBUG(3, "buf: %x %x %x %x %x %x\n",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

	return bfb_write_packets(fd, type, buffer, 6);
}


/**
	Send a cmd, subcmd packet, add chk (three word parameter).
 */
int bfb_write_subcmd3(fd_t fd, uint8_t type, uint8_t subtype, uint16_t p1, uint16_t p2, uint16_t p3)
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

	DEBUG(3, "buf: %x %x  %x %x  %x %x  %x %x\n",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

	return bfb_write_packets(fd, type, buffer, 8);
}


/**
	Send a cmd, subcmd packet, add long, word parameter.
 */
int bfb_write_subcmd_lw(fd_t fd, uint8_t type, uint8_t subtype, uint32_t p1, uint16_t p2)
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

	DEBUG(3, "buf: %02x  %02x %02x %02x %02x  %02x %02x\n",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

	return bfb_write_packets(fd, type, buffer, 7); /* no chk? */
}


/**
	Send actual packets.
	Patch from Jorge Ventura to handle EAGAIN from write.
 */
int bfb_write_packets(fd_t fd, uint8_t type, uint8_t *buffer, int length)
{
	bfb_frame_t *frame;
	int i;
	int l;

	struct timeval timeout;
	fd_set fds;
	int rc;
#ifdef _WIN32
	DWORD actual;
#else
	int actual;
#endif

#ifdef _WIN32
        return_val_if_fail (fd != INVALID_HANDLE_VALUE, FALSE);
#else
        return_val_if_fail (fd > 0, FALSE);
#endif
	/* select setup */
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	/* alloc frame buffer */
	frame = malloc((length > MAX_PACKET_DATA ? MAX_PACKET_DATA : length) + sizeof (bfb_frame_t));
	if (frame == NULL)
		return -1;

	for(i=0; i <length; i += MAX_PACKET_DATA) {

		l = length - i;
		if (l > MAX_PACKET_DATA)
			l = MAX_PACKET_DATA;

		frame->type = type;
		frame->len = l;
		frame->chk = frame->type ^ frame->len;

		memcpy(frame->payload, &buffer[i], l);

		/* Set time limit. */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		/* actual = bfb_io_write(fd, frame, l + sizeof (bfb_frame_t)); */
		rc = select(fd+1, NULL, &fds, NULL, &timeout);
		if ( rc > 0) {
#ifdef _WIN32
			if(!WriteFile(fd, frame, l + sizeof (bfb_frame_t), &actual, NULL))
				DEBUG(2, "%s() Write error: %ld\n", __func__, actual);
			DEBUG(3, "%s() Wrote %ld bytes (expected %d)\n", __func__, actual, l + sizeof (bfb_frame_t));
#else
			actual = write(fd, frame, l + sizeof (bfb_frame_t));
			DEBUG(3, "%s() Wrote %d bytes (expected %d)\n", __func__, actual, l + sizeof (bfb_frame_t));
			// delay for some USB-serial coverters (Gerhard Reithofer)
			// usleep(1000); // there has to be a better way
#endif
			if (actual < 0 || actual < l + (int) sizeof (bfb_frame_t)) {
				DEBUG(1, "%s() Write failed\n", __func__);
				free(frame);
				return -1;
			}
		} else { /* ! rc > 0*/
			DEBUG(1, "%s() Select failed\n", __func__);
			free(frame);
			return -1;
		}

	}
	free(frame);
	return i / MAX_PACKET_DATA;
}


/**
	Stuff data into packet buffers and send all packets.
 */
int bfb_send_data(fd_t fd, uint8_t type, uint8_t *data, uint16_t length, uint8_t seq)
{
	uint8_t *buffer;
	int actual;

	buffer = malloc(length + 7);
	if (buffer == NULL)
		return -1;

	actual = bfb_stuff_data(buffer, type, data, length, seq);
	DEBUG(3, "%s() Stuffed %d bytes\n", __func__, actual);

	actual = bfb_write_packets(fd, BFB_FRAME_DATA, buffer, actual);
	DEBUG(3, "%s() Wrote %d packets\n", __func__, actual);

	free(buffer);

	return actual;
}


/**
	Retrieve actual packets.
 */
/*@null@*/
bfb_frame_t *bfb_read_packets(uint8_t *buffer, int *length)
{
	bfb_frame_t *frame;
	int l;

	DEBUG(3, "%s() \n", __func__);

	if (*length < 0) {
		DEBUG(1, "%s() Wrong length?\n", __func__);
		return NULL;
	}

	if (*length == 0) {
		DEBUG(1, "%s() No packet?\n", __func__);
		return NULL;
	}

	if (*length < (int) sizeof(bfb_frame_t)) {
		DEBUG(1, "%s() Short packet?\n", __func__);
		return NULL;
	}
	
	/* temp frame */
	frame = (bfb_frame_t *)buffer;
	if ((frame->type ^ frame->len) != frame->chk) {
		DEBUG(1, "%s() Header error?\n", __func__);

		DEBUGBUFFER(buffer, *length);

		return NULL;
	}

	if (*length < frame->len + (int) sizeof(bfb_frame_t)) {
		DEBUG(2, "%s() Need more data?\n", __func__);
		return NULL;
	}

	/* copy frame from buffer */
	l = sizeof(bfb_frame_t) + frame->len;
	frame = malloc(l);
	if (frame == NULL)
		return NULL;
	memcpy(frame, buffer, l);

	/* remove frame from buffer */
	*length -= l;
	memmove(buffer, &buffer[l], *length);
	
	DEBUG(3, "%s() Packet 0x%02x (%d bytes)\n", __func__, frame->type, frame->len);
	return frame;
}


/**
	Append BFB frame to a data buffer.
 */
int	bfb_assemble_data(bfb_data_t **data, int *size, int *len, bfb_frame_t *frame)
{
	bfb_data_t *tmp;
	int l;

	DEBUG(3, "%s() \n", __func__);

	if (frame->type != BFB_FRAME_DATA) {
		DEBUG(1, "%s() Wrong frame type (0x%02x)?\n", __func__, frame->type);
		return -1;
	}

	/* temp data */
	tmp = (bfb_data_t *)frame->payload;
	if ((*len == 0) && (tmp->cmd == BFB_DATA_ACK)) {
		DEBUG(3, "%s() Skipping ack\n", __func__);
		return 0;
	}

	/* copy frame from buffer */
	DEBUG(3, "%s() data: %d + frame: %d\n", __func__, *len, frame->len);
	l = *len + frame->len;

	if (l > *size) {
		DEBUG(1, "%s() data buffer to small, realloc'ing\n", __func__);
		*data = realloc(*data, l);
		*size = l;
	}
	/* memcpy(ret, *data, *len); */
	memcpy(&((uint8_t *)*data)[*len], frame->payload, frame->len);

	/* free(*data); */
	*len = l;
	return 1;
}


/**
	Check if data buffer is complete and valid.
 */
int bfb_check_data(bfb_data_t *data, int len)
{
	int i;
        union {
                uint16_t value;
                uint8_t bytes[2];
        } l;

	DEBUG(3, "%s() \n", __func__);

	if (data == NULL)
		return -1;

	if (len < (int) sizeof(bfb_data_t))
		return 0;

	if (data->cmd != (uint8_t)~data->chk) {
		DEBUG(1, "%s() Broken data? 0x%02x, 0x%02x\n", __func__, data->cmd, (uint8_t)~data->chk);
		DEBUGBUFFER(data, len);
	}

	DEBUG(3, "%s() cmd: 0x%02x, chk: 0x%02x, seq: %d\n", __func__, data->cmd, data->chk, data->seq);

	if ((data->cmd != BFB_DATA_FIRST) && (data->cmd != BFB_DATA_NEXT)) {
		DEBUG(1, "%s() Wrong data type (0x%02x)?\n", __func__, data->cmd);
		return -1;
	}

	l.bytes[0] = data->len0;
	l.bytes[1] = data->len1;
	l.value = htons(l.value);

	DEBUG(3, "%s() fragment size %d, payload %d of indicated %d\n", __func__, len, len-sizeof(bfb_data_t), l.value);

	if (len-(int)sizeof(bfb_data_t) < (int)(l.value) + /*crc*/ 2)
		return 0;

	/* check CRC */
        l.value = INIT_FCS;

        for (i=2; i < len-2; i++) {
                l.value = irda_fcs(l.value, ((uint8_t *)data)[i]);
        }
        
        l.value = ~l.value;

	if ((((uint8_t *)data)[len-2] != l.bytes[0]) ||
	    (((uint8_t *)data)[len-1] != l.bytes[1])) {
		DEBUG(1, "%s() CRC-ERROR %02x %02x vs %02x %02x\n", __func__,
		      ((uint8_t *)data)[len-2], ((uint8_t *)data)[len-1],
		      l.bytes[0], l.bytes[1]);
	}

	DEBUG(2, "%s() data ready!\n", __func__);
	return 1;

}
