/**
	\file multicobex/multi_cobex.c
	Detect, initiate and run OBEX over custom serial port protocols
	(Siemens, Ericsson, New-Siemens, Motorola, Generic).
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#define usleep(t) Sleep((t) < 500 ? 1 : ((t) + 500) / 1000);
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <openobex/obex.h>
#include "multi_cobex.h"
#include "multi_cobex_private.h"
#include "multi_cobex_io.h"
#include "bfb/bfb.h"
#include <common.h>

static void cobex_cleanup(cobex_t *c, int force)
{
	return_if_fail (c != NULL);
	return_if_fail (c->fd != INVALID_HANDLE_VALUE);

	cobex_io_close(c->fd, c->type, force);

	c->fd = INVALID_HANDLE_VALUE;
}

/**
	Called from OBEX-lib to set up a connection.
 */
int cobex_connect(obex_t *self, void *data)
{
	cobex_t *c;
	return_val_if_fail (self != NULL, -1);
	return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

	DEBUG(3, "%s() \n", __func__);

	c->fd = cobex_io_open(c->tty, &c->type);
	DEBUG(3, "%s() cobex_io_open returned %d, %d\n", __func__, c->fd, c->type);

	if(c->fd == INVALID_HANDLE_VALUE)
		return -1;

	return 1;
}

/**
	Called from OBEX-lib to tear down a connection.
 */
int cobex_disconnect(obex_t *self, void *data)
{
	cobex_t *c = (cobex_t *)data;

	return_val_if_fail (self != NULL, -1);
	return_val_if_fail (data != NULL, -1);

	DEBUG(3, "%s() \n", __func__);
	cobex_cleanup(c, FALSE);
	return 1;
}

/**
	Called from OBEX-lib when data needs to be written.
 */
int cobex_write(obex_t *self, void *data, uint8_t *buffer, int length)
{
	int written = 0;
	cobex_t *c = (cobex_t *)data;

	return_val_if_fail (self != NULL, -1);
	return_val_if_fail (data != NULL, -1);

	DEBUG(3, "%s() \n", __func__);
	DEBUG(3, "%s() Data %d bytes\n", __func__, length);

	if (c->type == CT_BFB) {
		if (c->seq == 0){
			written = bfb_send_first(c->fd, buffer, length);
			DEBUG(2, "%s() Wrote %d first packets (%d bytes)\n", __func__, written, length);

		} else {
			written = bfb_send_next(c->fd, buffer, length, c->seq);
			DEBUG(2, "%s() Wrote %d packets (%d bytes)\n", __func__, written, length);
		}
		c->seq++;

	} else {
		int retries = 0;
		int fails = 0;

		for (retries = 0; written < length; retries++) {
			int chunk = write(c->fd, buffer+written, length-written);
			if (chunk <= 0) {
				if ( ++fails >= 10 ) { // to avoid infinite looping if something is really wrong
					DEBUG(1, "%s() Error writing to port (written %d bytes out of %d, in %d retries)\n",
					      __func__, written, length, retries);
					return written;
				}
				usleep(1); // This mysteriously avoids a resource not available error on write()
			} else {
				written += chunk;
				fails = 0; // Reset error counter on successful write op
			}
		}

		if (retries > 0)
			DEBUG(2, "%s() Wrote %d bytes in %d retries\n", __func__, written, retries);
	}

	return written;
}

/**
	Called when input data is needed.
 */
int cobex_handleinput(obex_t *self, void *data, int timeout)
{
	int actual;
	cobex_t *c = (cobex_t *)data;

	return_val_if_fail (self != NULL, -1);
	return_val_if_fail (data != NULL, -1);

	actual = cobex_io_read(c->fd, &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len, timeout);
	/* Check if this is a timeout (0) or error (-1) */
	if(actual <= 0)
		return actual;
	DEBUG(2, "%s() Read %d bytes (%d bytes already buffered)\n", __func__, actual, c->recv_len);

	if (c->type == CT_BFB) {
		if (c->data_buf == NULL || c->data_size == 0) {
			c->data_size = 1024;
			c->data_buf = malloc(c->data_size);
		}

		if (actual > 0) {
			bfb_frame_t *frame;

			c->recv_len += actual;
			DEBUGBUFFER(c->recv, c->recv_len);

			do {
				frame = bfb_read_packets(c->recv, &c->recv_len);
				if (frame == NULL)
					break;

				DEBUG(2, "%s() Parsed %x (%d bytes remaining)\n", __func__, frame->type, c->recv_len);

				(void)bfb_assemble_data(&c->data_buf, &c->data_size, &c->data_len, frame);

				if (bfb_check_data(c->data_buf, c->data_len) == 1) {
					actual = bfb_send_ack(c->fd);
					DEBUG(2, "%s() Wrote ack packet (%d)\n", __func__, actual);

					OBEX_CustomDataFeed(self, c->data_buf->data, c->data_len-7);
					c->data_len = 0;

					if (c->recv_len > 0) {
						DEBUG(2, "%s() Data remaining after feed, this can't be good.\n", __func__);
						DEBUGBUFFER(c->recv, c->recv_len);
					}

					return 1;
				}
			} while (1);
		}

	} else {
		if (actual > 0) {
			OBEX_CustomDataFeed(self, c->recv, actual);
			return 1;
		}
	}

	return actual;
}

/**
	Create a new multi cobex instance for a given TTY.

	\param tty the TTY to use. Defaults to the first serial TTY if NULL.
 */
obex_ctrans_t *cobex_ctrans (const char *tty) {
	obex_ctrans_t *ctrans;
	cobex_t *cobex;

	if(tty == NULL)
		tty = SERPORT;

	cobex = calloc(1, sizeof(*cobex));
	cobex->tty = strdup(tty);

	ctrans = calloc(1, sizeof(*ctrans));
	ctrans->connect     = cobex_connect;
	ctrans->disconnect  = cobex_disconnect;
	ctrans->write       = cobex_write;
	ctrans->listen      = NULL;
	ctrans->handleinput = cobex_handleinput;
	ctrans->customdata  = cobex;

	return ctrans;
}

/**
	Free all data related to a multi cobex instance.
 */
void cobex_free (obex_ctrans_t *ctrans)
{
	cobex_t *cobex;

	return_if_fail(ctrans != NULL);

	cobex = (cobex_t *)ctrans->customdata;

	return_if_fail(cobex != NULL);

	free(cobex->tty);
	cobex->tty = 0;

	free(cobex);
	cobex = 0;
	ctrans->customdata = 0;

	free (ctrans);
}
