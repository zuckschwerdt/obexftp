/*
 *                
 * Filename:      cobex_pe.c
 * Version:       0.2
 * Description:   Talk OBEX over a serial port (Ericsson specific)
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Wed Nov 17 22:05:16 1999
 * Modified at:   Mon, 29 Apr 2002 22:58:53 +0100
 * Modified by:   Christian W. Zuckschwerdt <zany@triq.net>
 *
 *   Copyright (c) 1998, 1999, Dag Brattli, All Rights Reserved.
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#define sleep(n)	_sleep(n*1000)
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include <openobex/obex.h>
#include "cobex_pe.h"
#include "cobex_pe_private.h"
#include <common.h>

#ifdef _WIN32
#else
/* Send an AT-command an expect 1 line back as answer */
static int cobex_do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen)
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

        return_val_if_fail (fd > 0, -1);
        return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	DEBUG(3, "%s() Sending %d: %s\n", __func__, cmdlen, cmd);

	/* Write command */
	if(write(fd, cmd, cmdlen) < cmdlen)	{
		DEBUG(1, "Error writing to port\n");
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

			DEBUG(3, "%s() tmpbuf=%d: %s\n", __func__, total, tmpbuf);

			/* Answer not found within 100 bytes. Cancel */
			if(total == sizeof(tmpbuf))
				return -1;

			if( (answer = strchr(tmpbuf, '\n')) )	{
				/* Remove first line (echo) */
				if( (answer_end = strchr(answer+1, '\n')) )	{
					/* Found end of answer */
					done = 1;
				}
			}
		}
		else	{
			/* Anser didn't come in time. Cancel */
			return -1;
		}
	}


/* 	DEBUG(3, "%s() buf:%08lx answer:%08lx end:%08lx\n", __func__, tmpbuf, answer, answer_end); */


	DEBUG(3, "%s() Answer: %s\n", __func__, answer);

	/* Remove heading and trailing \r */
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	DEBUG(3, "%s() Answer: %s\n", __func__, answer);

	answer_size = (answer_end) - answer +1;

	DEBUG(2, "%s() Answer size=%d\n", __func__, answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}
#endif

static void cobex_pe_cleanup(cobex_t *c, int force)
{
#ifdef _WIN32
#else
        return_if_fail (c != NULL);
        return_if_fail (c->fd > 0);

	if(force)	{
		/* Send a break to get out of OBEX-mode */
		if(ioctl(c->fd, TCSBRKP, 0) < 0)	{
			DEBUG(1, "Unable to send break!\n");
		}
		sleep(1);
	}
	close(c->fd);
	c->fd = -1;
#endif
}

#ifdef _WIN32
#else
/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
static int cobex_pe_init(cobex_t *c)
{
	struct termios oldtio, newtio;
	uint8_t rspbuf[200];

        return_val_if_fail (c != NULL, -1);

	DEBUG(3, "%s() \n", __func__);

	if( (c->fd = open(c->tty, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		DEBUG(1, "Can't open tty\n");
		return -1;
	}

	tcgetattr(c->fd, &oldtio);
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = B57600 | CS8 | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	tcflush(c->fd, TCIFLUSH);
	tcsetattr(c->fd, TCSANOW, &newtio);


	if(cobex_do_at_cmd(c->fd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error or already in OBEX mode\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(c->fd, "AT*EOBEX\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT*EOBEX (%s)\n", rspbuf);
		goto err;
	}

	return c->fd;
 err:
	cobex_pe_cleanup(c, TRUE);
	return -1;
}
#endif


int cobex_pe_connect(obex_t *self, void *data)
{
	cobex_t *c;
        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

	DEBUG(3, "%s() \n", __func__);

#ifdef _WIN32
#else
	if((c->fd = cobex_pe_init(c)) < 0)
#endif
		return -1;

	return 1;
}

int cobex_pe_disconnect(obex_t *self, void *data)
{
	cobex_t *c;
        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

	DEBUG(3, "%s() \n", __func__);
	cobex_pe_cleanup(c, FALSE);
	return 1;
}

/* Called from OBEX-lib when data needs to be written */
int cobex_pe_write(obex_t *self, void *data, uint8_t *buffer, int length)
{
#ifdef _WIN32
  return -1;
#else
	int actual;
	cobex_t *c;
        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;
	
	DEBUG(3, "%s() \n", __func__);

	DEBUG(2, "%s() Data %d bytes\n", __func__, length);

	actual = write(c->fd, buffer, length);
	if (actual < length)	{
		DEBUG(1, "Error writing to port\n");
		return -1;
	}

	return actual;
#endif
}

/* Called when input data is needed */
int cobex_pe_handleinput(obex_t *self, void *data, int timeout)
{
#ifdef _WIN32
  return 1;
#else
	int ret;
	struct timeval time;
	fd_set fdset;
	uint8_t recv[500];

	cobex_t *c;

        return_val_if_fail (self != NULL, -1);
        return_val_if_fail (data != NULL, -1);
	c = (cobex_t *) data;

	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(c->fd, &fdset);

	/* Wait for input */
	ret = select(c->fd + 1, &fdset, NULL, NULL, &time);

	DEBUG(2, "%s() There is something (%d)\n", __func__, ret);

	/* Check if this is a timeout (0) or error (-1) */
	if(ret <= 0)
		return ret;

	ret = read(c->fd, recv, sizeof(recv));
	DEBUG(2, "%s() Read %d bytes\n", __func__, ret);

	if (ret > 0) {
		OBEX_CustomDataFeed(self, recv, ret);
		return 1;
	}
	return ret;
#endif
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
obex_ctrans_t *cobex_pe_ctrans (const char *tty) {
	obex_ctrans_t *ctrans;
	cobex_t *cobex;

	cobex = calloc (1, sizeof(cobex_t));
	if(tty == NULL)
		tty = SERPORT;
	cobex->tty = strdup (tty);

	ctrans = calloc (1, sizeof(obex_ctrans_t));
	ctrans->connect = cobex_pe_connect;
	ctrans->disconnect = cobex_pe_disconnect;
	ctrans->write = cobex_pe_write;
	ctrans->listen = NULL;
	ctrans->handleinput = cobex_pe_handleinput;
	ctrans->customdata = cobex;
	
	return ctrans;
}


void cobex_pe_free (obex_ctrans_t *ctrans)
{
	cobex_t *cobex;

	return_if_fail (ctrans != NULL);

	cobex = (cobex_t *)ctrans->customdata;

	return_if_fail (cobex != NULL);

	free (cobex->tty);
	/* maybe there is a socket left? */
	/* close(cobex->fd); */

	free (cobex);
	free (ctrans);

	return;
}
