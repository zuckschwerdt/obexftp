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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#define sleep(n)        _sleep(n*1000)
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <glib.h>
#include <openobex/obex.h>
#include "obex_t.h"
#include "cobex_bfb.h"
#include "cobex_bfb_private.h"
#include <bfb/bfb.h>
#include <bfb/bfb_io.h>
#include <g_debug.h>

#undef G_LOG_DOMAIN
#define	G_LOG_DOMAIN	COBEX_BFB_LOG_DOMAIN

void cobex_cleanup(obex_t *self, int force)
{
        g_return_if_fail (self != NULL);
#ifdef _WIN32
        g_return_if_fail (OBEX_FD(self) != INVALID_HANDLE_VALUE);
#else
        g_return_if_fail (OBEX_FD(self) > 0);
#endif

       	bfb_io_close(OBEX_FD(self), force);

#ifdef _WIN32
	OBEX_FD(self) = INVALID_HANDLE_VALUE;
#else
	OBEX_FD(self) = -1;
#endif
}

gint cobex_connect(obex_t *self, gpointer userdata)
{
	cobex_t *c;
        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;

	g_debug(G_GNUC_FUNCTION "() \n");

#ifdef _WIN32
	if((OBEX_FD(self) = bfb_io_open(c->tty)) == INVALID_HANDLE_VALUE)
#else
	if((OBEX_FD(self) = bfb_io_open(c->tty)) < 0)
#endif
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
#ifdef _WIN32
	DWORD actual;
#else
	struct timeval time;
	fd_set fdset;
	gint actual;
#endif
	bfb_frame_t *frame;

	cobex_t *c;

        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;

#ifdef _WIN32
	if (!ReadFile(OBEX_FD(self), &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len, &actual, NULL))
		g_info(G_GNUC_FUNCTION "() Read error: %ld", actual);

	g_info(G_GNUC_FUNCTION "() Read %ld bytes (%d bytes already buffered)\n", actual, c->recv_len);
	/* FIXME ... */
#else
	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(OBEX_FD(self), &fdset);

	/* Wait for input */
	actual = select(OBEX_FD(self) + 1, &fdset, NULL, NULL, &time);

	g_info(G_GNUC_FUNCTION "() There is something (%d)\n", actual);

	/* Check if this is a timeout (0) or error (-1) */
	if(actual <= 0)
		return actual;

	actual = read(OBEX_FD(self), &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len);
	g_info(G_GNUC_FUNCTION "() Read %d bytes (%d bytes already buffered)\n", actual, c->recv_len);
#endif

	if (actual > 0) {
		c->recv_len += actual;

		while ((frame = bfb_read_packets(c->recv, &(c->recv_len)))) {
			g_info(G_GNUC_FUNCTION "() Parsed %x (%d bytes remaining)\n", frame->type, c->recv_len);

			c->data = bfb_assemble_data(c->data, &(c->data_len), frame);

			if (bfb_check_data(c->data, c->data_len) == 1) {
				actual = bfb_send_ack(OBEX_FD(self));
#ifdef _WIN32
				g_info(G_GNUC_FUNCTION "() Wrote ack packet (%ld)\n", actual);
#else
				g_info(G_GNUC_FUNCTION "() Wrote ack packet (%d)\n", actual);
#endif

				OBEX_CustomDataFeed(self, c->data->data, c->data_len-7);
				g_free(c->data);
				c->data = NULL;
				c->data_len = 0;

				return 1;
			}
		}

	}
	return actual;
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
