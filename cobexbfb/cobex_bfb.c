/*
 * cobex_bfb.c - Talk OBEX over a serial port (Siemens specific)
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
 *       Don, 17 Jan 2002 18:27:25 +0100
 * v0.6  Fre, 15 Feb 2002 15:41:10 +0100
 */

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
#include "cobex_bfb_private.h"
#include <g_debug.h>

#undef G_LOG_DOMAIN
#define	G_LOG_DOMAIN	COBEX_BFB_LOG_DOMAIN

/* Read a BFB answer */
static gint cobex_do_bfb_read(int fd, guint8 *buffer, gint length)
{
	struct timeval time;
	fd_set fdset;
	int actual;

        g_return_val_if_fail (fd > 0, -1);

	time.tv_sec = 2;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	if(select(fd+1, &fdset, NULL, NULL, &time)) {
		actual = read(fd, buffer, length);
		if(actual < 0)
			g_warning(G_GNUC_FUNCTION "() Read error: %d\n", actual);
		return actual;
	}
	else {
		g_info(G_GNUC_FUNCTION "() No data\n");
		return 0;
	}
}

/* Send an BFB init command an check for a valid answer frame */
static gboolean cobex_do_bfb_init(int fd)
{
	gint actual;
	bfb_frame_t *frame;
	guint8 rspbuf[200];

	guint8 init_magic = 0x14;
	guint8 init_magic2 = 0xaa;
	/*
	guint8 speed115200[] = {0xc0,'1','1','5','2','0','0',0x13,0xd2,0x2b};
	guint8 sifs[] = {'a','t','^','s','i','f','s',0x13};
	*/

        g_return_val_if_fail (fd > 0, FALSE);

	actual = bfb_write_packets (fd, BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));
	g_info(G_GNUC_FUNCTION "() Wrote %d packets\n", actual);

	if (actual < 1) {
		g_warning("BFB port error");
		return FALSE;
	}

	actual = cobex_do_bfb_read(fd, rspbuf, sizeof(rspbuf));
	g_info(G_GNUC_FUNCTION  "() Read %d bytes\n", actual);

	if (actual < 1) {
		g_warning("BFB read error");
		return FALSE;
	}

	frame = bfb_read_packets(rspbuf, &actual);
	g_info(G_GNUC_FUNCTION  "() Unstuffed, %d bytes remaining\n", actual);
	if (frame == NULL) {
		g_warning("BFB init error");
		return FALSE;
	}
	g_info("BFB init ok");

	if ((frame->len == 2) && (frame->payload[0] == init_magic) && (frame->payload[1] == init_magic2)) {
		g_free(frame);
		return TRUE;
	}

	g_warning("Error doing BFB init (%d, %x %x)",
		frame->len, frame->payload[0], frame->payload[1]);

	g_free(frame);

	return FALSE;
}

/* Send an AT-command an expect 1 line back as answer */
static gint cobex_do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen)
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

        g_return_val_if_fail (fd > 0, -1);
        g_return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	g_debug(G_GNUC_FUNCTION "() Sending %d: %s\n", cmdlen, cmd);

	// Write command
	if(write(fd, cmd, cmdlen) < cmdlen)	{
		g_warning("Error writing to port");
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

			g_debug(G_GNUC_FUNCTION "() tmpbuf=%d: %s\n", total, tmpbuf);

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


//	g_debug(G_GNUC_FUNCTION "() buf:%08lx answer:%08lx end:%08lx\n", tmpbuf, answer, answer_end);


	g_debug(G_GNUC_FUNCTION "() Answer: %s\n", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	g_debug(G_GNUC_FUNCTION "() Answer: %s\n", answer);

	answer_size = (answer_end) - answer +1;

	g_info(G_GNUC_FUNCTION "() Answer size=%d\n", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

void cobex_cleanup(obex_t *self, int force)
{
        g_return_if_fail (self != NULL);
        g_return_if_fail (OBEX_FD(self) > 0);

	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(OBEX_FD(self), TCSBRKP, 0) < 0)	{
			g_warning("Unable to send break!");
		}
		sleep(1);
	}
	close(OBEX_FD(self));
	OBEX_FD(self) = -1;
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
static gint cobex_init(obex_t *self, const gchar *ttyname)
{
	gint ttyfd;
	struct termios oldtio, newtio;
	guint8 rspbuf[200];

        g_return_val_if_fail (self != NULL, -1);

	g_debug(G_GNUC_FUNCTION "() \n");

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

	/* do we need to handle an error? */
	if (cobex_do_bfb_init (ttyfd)) {
		g_warning("Already in BFB mode.");
		goto bfbmode;
	}

	if(cobex_do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_warning("Comm-error or already in BFB mode");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_warning("Error doing ATZ (%s)", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_warning("Comm-error");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0)	{ // expect "OK" also!
		g_warning("Error doing AT^SIFS (%s)", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_warning("Comm-error");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_warning("Error doing AT^SBFB=1 (%s)", rspbuf);
		goto err;
	}

	sleep(1); // synch a bit
 bfbmode:
	if (! cobex_do_bfb_init (ttyfd)) {
		// well there may be some garbage -- just try again
		if (! cobex_do_bfb_init (ttyfd)) {
			g_warning("Couldn't init BFB mode.");
			goto err;
		}
	}

	return ttyfd;
 err:
	cobex_cleanup(self, TRUE);
	return -1;
}


gint cobex_connect(obex_t *self, gpointer userdata)
{
	cobex_t *c;
        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;

	g_debug(G_GNUC_FUNCTION "() \n");

	if((OBEX_FD(self) = cobex_init(self, c->tty)) < 0)
		return -1;

	return 1;
}

gint cobex_disconnect(obex_t *self, gpointer userdata)
{
	g_debug(G_GNUC_FUNCTION "() \n");
	cobex_cleanup(self, FALSE);
	return 1;
}

/* Called from OBEX-lib when data needs to be written */
gint cobex_write(obex_t *self, gpointer userdata, guint8 *buffer, gint length)
{
	int actual;
	cobex_t *c;
        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;
	
	g_debug(G_GNUC_FUNCTION "() \n");

	g_debug(G_GNUC_FUNCTION "() Data %d bytes\n", length);


	if (c->seq == 0){
		actual = bfb_send_first(OBEX_FD(self), buffer, length);
		g_info(G_GNUC_FUNCTION "() Wrote %d first packets (%d bytes)\n", actual, length);
	} else {
		actual = bfb_send_next(OBEX_FD(self), buffer, length, c->seq);
		g_info(G_GNUC_FUNCTION "() Wrote %d packets (%d bytes)\n", actual, length);
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

	cobex_t *c;

        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;

	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(OBEX_FD(self), &fdset);

	/* Wait for input */
	ret = select(OBEX_FD(self) + 1, &fdset, NULL, NULL, &time);

	g_info(G_GNUC_FUNCTION "() There is something (%d)\n", ret);

	/* Check if this is a timeout (0) or error (-1) */
	if(ret <= 0)
		return ret;

	ret = read(OBEX_FD(self), &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len);
	g_info(G_GNUC_FUNCTION "() Read %d bytes (%d bytes already buffered)\n", ret, c->recv_len);

	if (ret > 0) {
		c->recv_len += ret;

		while ((frame = bfb_read_packets(c->recv, &(c->recv_len)))) {
			g_info(G_GNUC_FUNCTION "() Parsed %x (%d bytes remaining)\n", frame->type, c->recv_len);

			c->data = bfb_assemble_data(c->data, &(c->data_len), frame);

			if (bfb_check_data(c->data, c->data_len) == 1) {
				ret = bfb_send_ack(OBEX_FD(self));
				g_info(G_GNUC_FUNCTION "() Wrote ack packet (%d)\n", ret);

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
/*
static obex_ctrans_t _cobex_ctrans = {
	connect: cobex_connect,
	disconnect: cobex_disconnect,
	write: cobex_write,
	listen: NULL,
	handleinput: cobex_handleinput,
};
*/
obex_ctrans_t *cobex_ctrans (const gchar *tty) {
	obex_ctrans_t *ctrans;
	cobex_t *cobex;

	cobex = g_new0 (cobex_t, 1);
	if(tty == NULL)
		tty = SERPORT;
	cobex->tty = g_strdup (tty);

	ctrans = g_new0 (obex_ctrans_t, 1);
	ctrans->connect = cobex_connect;
	ctrans->disconnect = cobex_disconnect;
	ctrans->write = cobex_write;
	ctrans->listen = NULL;
	ctrans->handleinput = cobex_handleinput;
	ctrans->userdata = cobex;
	
	return ctrans;
}


void cobex_free (obex_ctrans_t *ctrans)
{
	cobex_t *cobex;

	g_return_if_fail (ctrans != NULL);

	cobex = (cobex_t *)ctrans->userdata;

	g_return_if_fail (cobex != NULL);

	g_free (cobex->tty);
	/* maybe there is a bfb_data_t left? */
	/* g_free(cobex->data); */

	g_free (cobex);
	g_free (ctrans);

	return;
}
