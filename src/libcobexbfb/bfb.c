/*
 *                
 * Filename:      bfb.c
 * Version:       0.3
 * Description:   BFB transport encapsulation (Siemens specific)
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>

#include <glib.h>

#include <netinet/in.h>
#include "crc.h"
#include "bfb.h"
#include "debug.h"

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

/* send actual packets */
gint bfb_write_packets(int fd, guint8 type, guint8 *buffer, gint length)
{
	guint8 *packet;
	gint i;
	gint l;
	gint actual;
	
	// alloc packet buffer
	packet = g_malloc((length > MAX_PACKET_DATA ? MAX_PACKET_DATA : length) + 3);

	for(i=0; i <length; i += MAX_PACKET_DATA) {

		l = length - i;
		if (l > MAX_PACKET_DATA)
			l = MAX_PACKET_DATA;

		packet[0] = type;
		packet[1] = l;
		packet[2] = packet[0] ^ packet[1];

		memcpy(&packet[3], &buffer[i], l);

		actual = write(fd, packet, l + 3);
		
		DEBUG(3, G_GNUC_FUNCTION "() Wrote %d bytes (expected %d)\n", actual, l + 3);

	}
	g_free(packet);
	return i / MAX_PACKET_DATA;
}

gint bfb_send_data(int fd, guint8 type, guint8 *data, gint length, gint seq)
{
	guint8 *buffer;
	gint actual;

	buffer = g_malloc(length + 7);

	actual = bfb_stuff_data(buffer, type, data, length, seq);
	DEBUG(3, G_GNUC_FUNCTION "() Stuffed %d bytes\n", actual);

	actual = bfb_write_packets(fd, BFB_FRAME_DATA, buffer, actual);
	DEBUG(3, G_GNUC_FUNCTION "() Wrote %d packets\n", actual);

	return actual;
}


/* retrieve actual packets */
bfb_frame_t *bfb_read_packets(guint8 *buffer, gint *length)
{
	bfb_frame_t *frame;
	gint l;

	DEBUG(3, G_GNUC_FUNCTION "()\n");

	if (*length < 0) {
		DEBUG(1, G_GNUC_FUNCTION "() Wrong length?\n");
		return NULL;
	}

	if (*length == 0) {
		DEBUG(1, G_GNUC_FUNCTION "() No packet?\n");
		return NULL;
	}

	if (*length < sizeof(bfb_frame_t)) {
		DEBUG(1, G_GNUC_FUNCTION "() Short packet?\n");
		return NULL;
	}
	
	// temp frame
	frame = (bfb_frame_t *)buffer;
	if ((frame->type ^ frame->len) != frame->chk) {
		DEBUG(1, G_GNUC_FUNCTION "() Header error?\n");
		return NULL;
	}

	if (*length < frame->len + sizeof(bfb_frame_t)) {
		DEBUG(2, G_GNUC_FUNCTION "() Need more data?\n");
		return NULL;
	}

	// copy frame from buffer
	l = sizeof(bfb_frame_t) + frame->len;
	frame = g_malloc(l);
	memcpy(frame, buffer, l);

	// remove frame from buffer
	*length -= l;
	memmove(buffer, &buffer[l], *length);
	
	DEBUG(3, G_GNUC_FUNCTION "() Packet %x (%d bytes)\n", frame->type, frame->len);
	return frame;
}

bfb_data_t *bfb_assemble_data(bfb_data_t *data, gint *fraglen, bfb_frame_t *frame)
{
	bfb_data_t *ret;
	gint l;

	DEBUG(3, G_GNUC_FUNCTION "()\n");

	if (frame->type != BFB_FRAME_DATA) {
		g_print(__FUNCTION__ "() Wrong frame type (%x)?\n", frame->type);
		return data;
	}

	// temp data
	ret = (bfb_data_t *)frame->payload;
	if (ret->cmd == BFB_DATA_ACK) {
		DEBUG(3, G_GNUC_FUNCTION "() Skipping ack\n");
		return data;
	}
	/*
	if ((ret->cmd != BFB_DATA_FIRST) && (ret->cmd != BFB_DATA_NEXT)) {
		g_print(__FUNCTION__ "() Wrong data type (%x)?\n", ret->cmd);
		return data;
	}
	*/

	// copy frame from buffer
	DEBUG(3, G_GNUC_FUNCTION "() data: %d, frame: %d\n", *fraglen, frame->len);
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

	DEBUG(3, G_GNUC_FUNCTION "()\n");

	if (data == NULL)
		return -1;

	l.bytes[0] = data->len0;
	l.bytes[1] = data->len1;
	l.value = htons(l.value);

	DEBUG(3, G_GNUC_FUNCTION "() fragment size is %d\n", fraglen);
	DEBUG(3, G_GNUC_FUNCTION "() expected len %d\n", l.value);
	DEBUG(3, G_GNUC_FUNCTION "() data size is %d\n", fraglen-sizeof(bfb_data_t));

	if (fraglen-sizeof(bfb_data_t) + 2 < l.value)
		return 0;

/*
	// check CRC
	if (sizeof(data) < l.value)
		return -1;
*/

	DEBUG(2, G_GNUC_FUNCTION "() data ready!\n");
	return 1;

}
