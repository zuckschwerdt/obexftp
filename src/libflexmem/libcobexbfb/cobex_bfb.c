/*
 *                
 * Filename:      cobex_bfb.c
 * Version:       0.4
 * Description:   Talk OBEX over a serial port (Siemens specific)
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
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

#define SERPORT "/dev/ttyS0"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <glib.h>
#include <openobex/obex.h>
#include "obex_t.h"
#include "cobex_bfb.h"
#include "debug.h"


obex_t *cobex_handle;
cobex_t *c;
gchar *_tty = NULL;

gint cobex_set_tty(gchar *tty)
{
	g_free(_tty);
	_tty = tty;

	return 0;
}

/* Read a BFB answer */
gint cobex_do_bfb_read(int fd, guint8 *buffer, gint length)
{
	struct timeval time;
	fd_set fdset;
	int actual;

	time.tv_sec = 5;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	if(select(fd+1, &fdset, NULL, NULL, &time)) {
		actual = read(fd, buffer, length);
		if(actual < 0)
			DEBUG(1, G_GNUC_FUNCTION "() Read error: %d\n", actual);
		return actual;
	}
	else {
		DEBUG(1, G_GNUC_FUNCTION "() No data\n");
		return 0;
	}
}


/* Send an AT-command an expect 1 line back as answer */
int cobex_do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen)
{
	fd_set ttyset;
	struct timeval tv;

	char *answer;
	char *answer_end = NULL;
	unsigned int answer_size;

	char tmpbuf[100] = {0,};
	int actual;
	int total = 0;
	int done = 0;
	int cmdlen;

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	DEBUG(4, G_GNUC_FUNCTION "() Sending %d: %s\n", cmdlen, cmd);

	// Write command
	if(write(fd, cmd, cmdlen) < cmdlen)	{
		g_print("Error writing to port");
		return -1;
	}

	while(!done)	{
		FD_ZERO(&ttyset);
		FD_SET(fd, &ttyset);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
			actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
			if(actual < 0)
				return actual;
			total += actual;

			DEBUG(4, G_GNUC_FUNCTION "() tmpbuf=%d: %s\n", total, tmpbuf);

			// Answer not found within 100 bytes. Cancel
			if(total == sizeof(tmpbuf))
				return -1;

			if( (answer = index(tmpbuf, '\n')) )	{
				// Remove first line (echo)
				if( (answer_end = index(answer+1, '\n')) )	{
					// Found end of answer
					done = 1;
				}
			}
		}
		else	{
			// Anser didn't come in time. Cancel
			return -1;
		}
	}


//	DEBUG(4, G_GNUC_FUNCTION "() buf:%08lx answer:%08lx end:%08lx\n", tmpbuf, answer, answer_end);


	DEBUG(4, G_GNUC_FUNCTION "() Answer: %s\n", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	DEBUG(4, G_GNUC_FUNCTION "() Answer: %s\n", answer);

	answer_size = (answer_end) - answer +1;

	DEBUG(1, G_GNUC_FUNCTION "() Answer size=%d\n", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
gint cobex_init(char *ttyname)
{
	int ttyfd;
	struct termios oldtio, newtio;

	int actual;
	bfb_frame_t *frame;
	guint8 rspbuf[200];

	guint8 init_magic = 0x14;
	guint8 init_magic2 = 0xaa;
	/*
	guint8 speed[] = {0xc0,'1','1','5','2','0','0',0x13,0xd2,0x2b};
	guint8 sifs[] = {'a','t','^','s','i','f','s',0x13};
	*/

	DEBUG(1, G_GNUC_FUNCTION "() \n");

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		g_error("Can' t open tty");
		return -1;
	}

	tcgetattr(ttyfd, &oldtio);
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = B57600 | CS8 | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	tcflush(ttyfd, TCIFLUSH);
	tcsetattr(ttyfd, TCSANOW, &newtio);

	if(cobex_do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_print("Comm-error\n");
		goto bfbmode;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_print("Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_print("Comm-error\n");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0)	{ // expect "OK" also!
		g_print("Error doing AT^SIFS (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_print("Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_print("Error doing AT^SBFB=1 (%s)\n", rspbuf);
		goto err;
	}

	sleep(1); // synch a bit
 bfbmode:
	actual = bfb_write_packets(ttyfd, BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));
	DEBUG(1, G_GNUC_FUNCTION "() Wrote %d packets\n", actual);
	actual = cobex_do_bfb_read(ttyfd, rspbuf, sizeof(rspbuf));
	DEBUG(1, G_GNUC_FUNCTION  "() Read %d bytes\n", actual);
	frame = bfb_read_packets(rspbuf, &actual);
	DEBUG(2, G_GNUC_FUNCTION  "() Unstuffed, %d bytes remaining\n", actual);

	if ((frame->len != 2) || (frame->payload[0] != init_magic) || (frame->payload[1] != init_magic2)) {
		g_print("Error doing BFB init (%d, %x %x)\n",
		       frame->len, frame->payload[0], frame->payload[1]);
		goto err;
	}
	g_free(frame);


//	actual = bfb_write_packets(ttyfd, BFB_FRAME_INTERFACE, speed, sizeof(speed));

//	actual = bfb_write_packets(ttyfd, BFB_FRAME_AT, sifs, sizeof(sifs));



	return ttyfd;
 err:
	cobex_cleanup(TRUE);
	return -1;
}

void cobex_cleanup(int force)
{
	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(OBEX_FD(cobex_handle), TCSBRKP, 0) < 0)	{
			g_print("Unable to send break!\n");
		}
		sleep(1);
	}
	close(OBEX_FD(cobex_handle));
	OBEX_FD(cobex_handle) = -1;
}


gint cobex_connect(obex_t *self, gpointer userdata)
{
	DEBUG(1, G_GNUC_FUNCTION "() \n");

	cobex_handle = self;

	c = g_new0(cobex_t, 1);

	if(_tty == NULL)
		_tty = SERPORT;

	if((OBEX_FD(self) = cobex_init(_tty)) < 0)
		return -1;

	return 1;
}

gint cobex_disconnect(obex_t *self, gpointer userdata)
{
	DEBUG(1, G_GNUC_FUNCTION "() \n");
	cobex_cleanup(FALSE);
	return 1;
}

/* Called from OBEX-lib when data needs to be written */
gint cobex_write(obex_t *self, gpointer userdata, guint8 *buffer, gint length)
{
	int actual;
	
	DEBUG(1, G_GNUC_FUNCTION "() \n");

	DEBUG(1, G_GNUC_FUNCTION "() Data %d bytes\n", length);


	if (c->seq == 0){
		actual = bfb_send_first(OBEX_FD(self), buffer, length);
		DEBUG(2, G_GNUC_FUNCTION "() Wrote %d first packets (%d bytes)\n", actual, length);
	} else {
		actual = bfb_send_next(OBEX_FD(self), buffer, length, c->seq);
		DEBUG(2, G_GNUC_FUNCTION "() Wrote %d packets (%d bytes)\n", actual, length);
	}
	c->seq++;


	return actual;
}

/* Called when input data is needed */
gint cobex_handleinput(obex_t *self, gpointer userdata, gint timeout)
{
	gint ret;
	struct timeval time;
	fd_set fdset;
	bfb_frame_t *frame;

	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(OBEX_FD(self), &fdset);

	/* Wait for input */
	ret = select(OBEX_FD(self) + 1, &fdset, NULL, NULL, &time);

	DEBUG(1, G_GNUC_FUNCTION "() There is something (%d)\n", ret);

	/* Check if this is a timeout (0) or error (-1) */
	if(ret <= 0)
		return ret;

	ret = read(OBEX_FD(self), &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len);
	DEBUG(1, G_GNUC_FUNCTION "() Read %d bytes (%d bytes already buffered)\n", ret, c->recv_len);

	if (ret > 0) {
		c->recv_len += ret;

		while ((frame = bfb_read_packets(c->recv, &(c->recv_len)))) {
			DEBUG(1, G_GNUC_FUNCTION "() Parsed %x (%d bytes remaining)\n", frame->type, c->recv_len);

			c->data = bfb_assemble_data(c->data, &(c->data_len), frame);

			if (bfb_check_data(c->data, c->data_len) == 1) {
				ret = bfb_send_ack(OBEX_FD(self));
				DEBUG(2, G_GNUC_FUNCTION "() Wrote ack packet (%d)\n", ret);

				OBEX_CustomDataFeed(self, c->data->data, c->data_len-7);
				g_free(c->data);
				c->data = NULL;
				c->data_len = 0;

				return 1;
			}
		}

	}
	return ret;
}


/* Well, maybe this should be somewhere in the header */

static obex_ctrans_t _cobex_ctrans = {
	connect: cobex_connect,
	disconnect: cobex_disconnect,
	write: cobex_write,
	listen: NULL,
	handleinput: cobex_handleinput,
};

obex_ctrans_t *cobex_ctrans(void) {
	return &_cobex_ctrans;
}
