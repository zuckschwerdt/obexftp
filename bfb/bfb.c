/*
 * bfb.c - BFB transport encapsulation (used for Siemens mobile equipment)
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
/*
 * v0.1:  Die,  5 Feb 2002 22:46:19 +0100
 * v0.4:  Don, 25 Jul 2002 03:16:41 +0200
 */

#include <glib.h>

#include <string.h>
#include <unistd.h>

#include "crc.h"
#include "bfb.h"
#include <g_debug.h>

#undef G_LOG_DOMAIN
#define	G_LOG_DOMAIN	BFB_LOG_DOMAIN

/* returns the whole buffer folded with xor. */
guint8 bfb_checksum(guint8 *data, gint len)
{
	int i;
	guint8 chk = 0;
      
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
gint bfb_stuff_data(guint8 *buffer, guint8 type, guint8 *data, gint len, gint seq)
{
        int i;
        union {
                guint16 value;
                guint8 bytes[2];
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

	fcs.value = g_htons(len);
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
	//fcs.value = g_htons(fcs.value);
	buffer[len+5] = fcs.bytes[0];
	buffer[len+6] = fcs.bytes[1];

	return len+7;
}

/* send a cmd, subcmd packet, add chk (no parameters) */
gint bfb_write_subcmd(int fd, guint8 type, guint8 subtype)
{
	guint8 buffer[2];

	buffer[0] = subtype;
	buffer[1] = bfb_checksum(buffer, 1);

	return bfb_write_packets(fd, type, buffer, 2);
}

/* send a cmd, subcmd packet */
gint bfb_write_subcmd0(int fd, guint8 type, guint8 subtype)
{
	return bfb_write_packets(fd, type, &subtype, 1);
}

/* send a cmd, subcmd, data packet */
gint bfb_write_subcmd8(int fd, guint8 type, guint8 subtype, guint8 p1)
{
	guint8 buffer[2];

	buffer[0] = subtype;
	buffer[1] = p1;

	return bfb_write_packets(fd, type, buffer, 2);
}

/* send a cmd, subcmd packet, add chk (one word parameter) */
gint bfb_write_subcmd1(int fd, guint8 type, guint8 subtype, guint16 p1)
{
	guint8 buffer[4];

	buffer[0] = subtype;

	p1 = GUINT16_TO_LE(p1);	 /* mobile need little-endianess always */
	buffer[1] = G_STRUCT_MEMBER(guint8, &p1, 0);
	buffer[2] = G_STRUCT_MEMBER(guint8, &p1, 1);

	buffer[3] = bfb_checksum(buffer, 3);

	g_debug("buf: %x %x %x %x",
	      buffer[0], buffer[1], buffer[2], buffer[3]);
	return bfb_write_packets(fd, type, buffer, 4);
}

/* send a cmd, subcmd packet, add chk (two word parameter) */
gint bfb_write_subcmd2(int fd, guint8 type, guint8 subtype, guint16 p1, guint16 p2)
{
	guint8 buffer[6];

	buffer[0] = subtype;

	p1 = GUINT16_TO_LE(p1);	 /* mobile need little-endianess always */
	buffer[1] = G_STRUCT_MEMBER(guint8, &p1, 0);
	buffer[2] = G_STRUCT_MEMBER(guint8, &p1, 1);
	p2 = GUINT16_TO_LE(p2);	 /* mobile need little-endianess always */
	buffer[3] = G_STRUCT_MEMBER(guint8, &p2, 0);
	buffer[4] = G_STRUCT_MEMBER(guint8, &p2, 1);

	buffer[5] = bfb_checksum(buffer, 5);

	g_debug("buf: %x %x %x %x %x %x",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

	return bfb_write_packets(fd, type, buffer, 6);
}

/* send a cmd, subcmd packet, add chk (three word parameter) */
gint bfb_write_subcmd3(int fd, guint8 type, guint8 subtype, guint16 p1, guint16 p2, guint16 p3)
{
	guint8 buffer[8];

	buffer[0] = subtype;

	p1 = GUINT16_TO_LE(p1);	 /* mobile need little-endianess always */
	buffer[1] = G_STRUCT_MEMBER(guint8, &p1, 0);
	buffer[2] = G_STRUCT_MEMBER(guint8, &p1, 1);
	p2 = GUINT16_TO_LE(p2);	 /* mobile need little-endianess always */
	buffer[3] = G_STRUCT_MEMBER(guint8, &p2, 0);
	buffer[4] = G_STRUCT_MEMBER(guint8, &p2, 1);
	p3 = GUINT16_TO_LE(p3);	 /* mobile need little-endianess always */
	buffer[5] = G_STRUCT_MEMBER(guint8, &p3, 0);
	buffer[6] = G_STRUCT_MEMBER(guint8, &p3, 1);

	buffer[7] = bfb_checksum(buffer, 7);

	g_debug("buf: %x %x  %x %x  %x %x  %x %x",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

	return bfb_write_packets(fd, type, buffer, 8);
}

/* send a cmd, subcmd packet, add long, word parameter */
gint bfb_write_subcmd_lw(int fd, guint8 type, guint8 subtype, guint32 p1, guint16 p2)
{
	guint8 buffer[8];

	buffer[0] = subtype;

	p1 = GUINT32_TO_LE(p1);	 /* mobile need little-endianess always */
	buffer[1] = G_STRUCT_MEMBER(guint8, &p1, 0);
	buffer[2] = G_STRUCT_MEMBER(guint8, &p1, 1);
	buffer[3] = G_STRUCT_MEMBER(guint8, &p1, 2);
	buffer[4] = G_STRUCT_MEMBER(guint8, &p1, 3);
	p2 = GUINT16_TO_LE(p2);	 /* mobile need little-endianess always */
	buffer[5] = G_STRUCT_MEMBER(guint8, &p2, 0);
	buffer[6] = G_STRUCT_MEMBER(guint8, &p2, 1);

	buffer[7] = bfb_checksum(buffer, 7);

	g_debug("buf: %02x  %02x %02x %02x %02x  %02x %02x",
	      buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);

	return bfb_write_packets(fd, type, buffer, 7); /* no chk? */
}


/* send actual packets */
gint bfb_write_packets(int fd, guint8 type, guint8 *buffer, gint length)
{
	bfb_frame_t *frame;
	gint i;
	gint l;
	gint actual;
	
	// alloc frame buffer
	frame = g_malloc((length > MAX_PACKET_DATA ? MAX_PACKET_DATA : length) + sizeof (bfb_frame_t));

	for(i=0; i <length; i += MAX_PACKET_DATA) {

		l = length - i;
		if (l > MAX_PACKET_DATA)
			l = MAX_PACKET_DATA;

		frame->type = type;
		frame->len = l;
		frame->chk = frame->type ^ frame->len;

		memcpy(frame->payload, &buffer[i], l);

		actual = write(fd, frame, l + sizeof (bfb_frame_t));

		g_debug(G_GNUC_FUNCTION "() Wrote %d bytes (expected %d)", actual, l + sizeof (bfb_frame_t));

		if (actual < 0 || actual < l + sizeof (bfb_frame_t)) {
			g_warning(G_GNUC_FUNCTION "() Write failed");
			g_free(frame);
			return -1;
		}

	}
	g_free(frame);
	return i / MAX_PACKET_DATA;
}

gint bfb_send_data(int fd, guint8 type, guint8 *data, gint length, gint seq)
{
	guint8 *buffer;
	gint actual;

	buffer = g_malloc(length + 7);

	actual = bfb_stuff_data(buffer, type, data, length, seq);
	g_debug(G_GNUC_FUNCTION "() Stuffed %d bytes", actual);

	actual = bfb_write_packets(fd, BFB_FRAME_DATA, buffer, actual);
	g_debug(G_GNUC_FUNCTION "() Wrote %d packets", actual);

	return actual;
}


/* retrieve actual packets */
bfb_frame_t *bfb_read_packets(guint8 *buffer, gint *length)
{
	bfb_frame_t *frame;
	gint l;

	g_debug(G_GNUC_FUNCTION "() ");

	if (*length < 0) {
		g_warning(G_GNUC_FUNCTION "() Wrong length?");
		return NULL;
	}

	if (*length == 0) {
		g_warning(G_GNUC_FUNCTION "() No packet?");
		return NULL;
	}

	if (*length < sizeof(bfb_frame_t)) {
		g_warning(G_GNUC_FUNCTION "() Short packet?");
		return NULL;
	}
	
	// temp frame
	frame = (bfb_frame_t *)buffer;
	if ((frame->type ^ frame->len) != frame->chk) {
		g_warning(G_GNUC_FUNCTION "() Header error?");
		return NULL;
	}

	if (*length < frame->len + sizeof(bfb_frame_t)) {
		g_warning(G_GNUC_FUNCTION "() Need more data?");
		return NULL;
	}

	// copy frame from buffer
	l = sizeof(bfb_frame_t) + frame->len;
	frame = g_malloc(l);
	memcpy(frame, buffer, l);

	// remove frame from buffer
	*length -= l;
	memmove(buffer, &buffer[l], *length);
	
	g_debug(G_GNUC_FUNCTION "() Packet %x (%d bytes)", frame->type, frame->len);
	return frame;
}

bfb_data_t *bfb_assemble_data(bfb_data_t *data, gint *fraglen, bfb_frame_t *frame)
{
	bfb_data_t *ret;
	gint l;

	g_debug(G_GNUC_FUNCTION "() ");

	if (frame->type != BFB_FRAME_DATA) {
		g_warning(__FUNCTION__ "() Wrong frame type (%x)?", frame->type);
		return data;
	}

	// temp data
	ret = (bfb_data_t *)frame->payload;
	if (ret->cmd == BFB_DATA_ACK) {
		g_debug(G_GNUC_FUNCTION "() Skipping ack");
		return data;
	}
	/*
	if ((ret->cmd != BFB_DATA_FIRST) && (ret->cmd != BFB_DATA_NEXT)) {
		g_warning(__FUNCTION__ "() Wrong data type (%x)?", ret->cmd);
		return data;
	}
	*/

	// copy frame from buffer
	g_debug(G_GNUC_FUNCTION "() data: %d, frame: %d", *fraglen, frame->len);
	l = *fraglen + frame->len;
	ret = g_realloc(data, l);
	//memcpy(ret, data, *fraglen);
	memcpy(&((guint8 *)ret)[*fraglen], frame->payload, frame->len);

	//g_free(data);
	*fraglen = l;
	return ret;
}

gint bfb_check_data(bfb_data_t *data, gint fraglen)
{
        union {
                guint16 value;
                guint8 bytes[2];
        } l;

	g_debug(G_GNUC_FUNCTION "() ");

	if (data == NULL)
		return -1;

	l.bytes[0] = data->len0;
	l.bytes[1] = data->len1;
	l.value = g_htons(l.value);

	g_debug(G_GNUC_FUNCTION "() fragment size is %d", fraglen);
	g_debug(G_GNUC_FUNCTION "() expected len %d", l.value);
	g_debug(G_GNUC_FUNCTION "() data size is %d", fraglen-sizeof(bfb_data_t));

	if (fraglen-sizeof(bfb_data_t) + 2 < l.value)
		return 0;

/*
	// check CRC
	if (sizeof(data) < l.value)
		return -1;
*/

	g_info(G_GNUC_FUNCTION "() data ready!");
	return 1;

}
