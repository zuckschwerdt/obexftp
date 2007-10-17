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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#define sleep(t) Sleep((t) * 1000)
#define usleep(t) Sleep((t) < 500 ? 1 : ((t) + 500) / 1000);
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <openobex/obex.h>
#include "multi_cobex.h"
#include "multi_cobex_private.h"
#include <bfb/bfb.h>
#include <bfb/bfb_io.h>
#include <common.h>

static void cobex_cleanup(cobex_t *c, int force)
{
        return_if_fail (c != NULL);
#ifdef _WIN32
        return_if_fail (c->fd != INVALID_HANDLE_VALUE);
#else
        return_if_fail (c->fd > 0);
#endif

	if (c->type == CT_BFB) {
		/* needed to leave transparent OBEX(3) mode 
		 * and enter RCCP(0) mode. 
		 * DCA-540 needs cable replug, probably problem 
		 * with the linux driver (linux-2.6.17.13) 
		 */
		bfb_write_at(c->fd, "AT^SBFB=0\r");
		sleep(1);
		bfb_io_write(c->fd, "+++", 3);
		sleep(1);
		bfb_io_write(c->fd,"\r", 1);
	}
       	bfb_io_close(c->fd, force);

#ifdef _WIN32
	c->fd = INVALID_HANDLE_VALUE;
#else
	c->fd = -1;
#endif
}

/**
	Called from OBEX-lib to set up a connection.
 */
int cobex_connect(obex_t *self, void *data)
{
	cobex_t *c;
	enum trans_type typeinfo;
        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

	DEBUG(3, "%s() \n", __func__);

	c->fd = bfb_io_open(c->tty, &typeinfo);
	DEBUG(3, "%s() bfb_io_open returned %d, %d\n", __func__, c->fd, typeinfo);
	switch (typeinfo) {
	case TT_BFB:
		c->type = CT_BFB;
		break;
	case TT_ERICSSON:
		c->type = CT_ERICSSON;
		break;
	case TT_SIEMENS:
		c->type = CT_SIEMENS;
		break;
	case TT_MOTOROLA:
		c->type = CT_MOTOROLA;
		break;
	case TT_GENERIC:
		c->type = CT_GENERIC;
		break;
	default:
		c->type = 0; /* invalid */
		return -1;
	}

#ifdef _WIN32
	if(c->fd == INVALID_HANDLE_VALUE)
#else
	if(c->fd == -1)
#endif
		return -1;

	return 1;
}

/**
	Called from OBEX-lib to tear down a connection.
 */
int cobex_disconnect(obex_t *self, void *data)
{
	cobex_t *c;
        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

	DEBUG(3, "%s() \n", __func__);
	cobex_cleanup(c, FALSE);
	return 1;
}

/**
	Called from OBEX-lib when data needs to be written.
 */
int cobex_write(obex_t *self, void *data, uint8_t *buffer, int length)
{
	int written;
	cobex_t *c;
        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;
	
	DEBUG(3, "%s() \n", __func__);

	DEBUG(3, "%s() Data %d bytes\n", __func__, length);

	if (c->type != CT_BFB) {
		int retries=0, chunk, fails=0;
		written = 0;
		for (retries = 0; written < length; retries++) {
			chunk = write(c->fd, buffer+written, length-written);
			if (chunk <= 0) {
				if ( ++fails >= 10 ) { // to avoid infinite looping if something is really wrong
					DEBUG(1, "%s() Error writing to port (written %d bytes out of %d, in %d retries)\n", __func__, written, length, retries);
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
		return written;
	}

	if (c->seq == 0){
		written = bfb_send_first(c->fd, buffer, length);
		DEBUG(2, "%s() Wrote %d first packets (%d bytes)\n", __func__, written, length);
	} else {
		written = bfb_send_next(c->fd, buffer, length, c->seq);
		DEBUG(2, "%s() Wrote %d packets (%d bytes)\n", __func__, written, length);
	}
	c->seq++;

	return written;
}

/**
	Called when input data is needed.
 */
int cobex_handleinput(obex_t *self, void *data, int timeout)
{
#ifdef _WIN32
	DWORD actual;
#else
	struct timeval time;
	fd_set fdset;
	int actual;
#endif
	bfb_frame_t *frame;

	cobex_t *c;

        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

#ifdef _WIN32
	if (!ReadFile(c->fd, &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len, &actual, NULL))
		DEBUG(2, "%s() Read error: %ld\n", __func__, actual);

	DEBUG(2, "%s() Read %ld bytes (%d bytes already buffered)\n", __func__, actual, c->recv_len);
	/* FIXME ... */
#else
	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(c->fd, &fdset);

	/* Wait for input */
	actual = select(c->fd + 1, &fdset, NULL, NULL, &time);

	DEBUG(2, "%s() There is something (%d)\n", __func__, actual);

	/* Check if this is a timeout (0) or error (-1) */
	if(actual <= 0)
		return actual;

	actual = read(c->fd, &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len);
	DEBUG(2, "%s() Read %d bytes (%d bytes already buffered)\n", __func__, actual, c->recv_len);
#endif

	if (c->type != CT_BFB) {
		if (actual > 0) {
			OBEX_CustomDataFeed(self, c->recv, actual);
			return 1;
		}
		return actual;
	}

	if ((c->data_buf == NULL) || (c->data_size == 0)) {
		c->data_size = 1024;
		c->data_buf = malloc(c->data_size);
	}

	if (actual > 0) {
		c->recv_len += actual;
		DEBUGBUFFER(c->recv, c->recv_len);

		while ((frame = bfb_read_packets(c->recv, &(c->recv_len)))) {
			DEBUG(2, "%s() Parsed %x (%d bytes remaining)\n", __func__, frame->type, c->recv_len);

			(void) bfb_assemble_data(&c->data_buf, &c->data_size, &c->data_len, frame);

			if (bfb_check_data(c->data_buf, c->data_len) == 1) {
				actual = bfb_send_ack(c->fd);
#ifdef _WIN32
				DEBUG(2, "%s() Wrote ack packet (%ld)\n", __func__, actual);
#else
				DEBUG(2, "%s() Wrote ack packet (%d)\n", __func__, actual);
#endif

				OBEX_CustomDataFeed(self, c->data_buf->data, c->data_len-7);
				/*
				free(c->data);
				c->data = NULL;
				*/
				c->data_len = 0;

				if (c->recv_len > 0) {
					DEBUG(2, "%s() Data remaining after feed, this can't be good.\n", __func__);
					DEBUGBUFFER(c->recv, c->recv_len);
				}

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

/**
	Create a new multi cobex instance for a given TTY.

	\param tty the TTY to use. Defaults to the first serial TTY if NULL.
 */
obex_ctrans_t *cobex_ctrans (const char *tty) {
	obex_ctrans_t *ctrans;
	cobex_t *cobex;

	cobex = calloc (1, sizeof(cobex_t));
	if(tty == NULL)
		tty = SERPORT;
	cobex->tty = strdup (tty);

	ctrans = calloc (1, sizeof(obex_ctrans_t));
	ctrans->connect = cobex_connect;
	ctrans->disconnect = cobex_disconnect;
	ctrans->write = cobex_write;
	ctrans->listen = NULL;
	ctrans->handleinput = cobex_handleinput;
	ctrans->customdata = cobex;
	
	return ctrans;
}

/**
	Free all data related to a multi cobex instance.
 */
void cobex_free (obex_ctrans_t *ctrans)
{
	cobex_t *cobex;

	return_if_fail (ctrans != NULL);

	cobex = (cobex_t *)ctrans->customdata;

	return_if_fail (cobex != NULL);

	free (cobex->tty);
	/* maybe there is a bfb_data_t left? */
	/* free(cobex->data); */

	free (cobex);
	free (ctrans);

	return;
}
