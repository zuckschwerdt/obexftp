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
#include "cobex_pe.h"
#include "cobex_pe_private.h"
#include "debug.h"


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
	DEBUG(4, G_GNUC_FUNCTION "() Sending %d: %s\n", cmdlen, cmd);

	// Write command
	if(write(fd, cmd, cmdlen) < cmdlen)	{
		g_print("Error writing to port\n");
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

static void cobex_pe_cleanup(obex_t *self, int force)
{
        g_return_if_fail (self != NULL);
        g_return_if_fail (OBEX_FD(self) > 0);

	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(OBEX_FD(self), TCSBRKP, 0) < 0)	{
			g_print("Unable to send break!\n");
		}
		sleep(1);
	}
	close(OBEX_FD(self));
	OBEX_FD(self) = -1;
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
static gint cobex_pe_init(obex_t *self, const gchar *ttyname)
{
	gint ttyfd;
	struct termios oldtio, newtio;
	guint8 rspbuf[200];

        g_return_val_if_fail (self != NULL, -1);

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
		g_print("Comm-error or already in OBEX mode\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_print("Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT*EOBEX\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_print("Comm-error\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		printf("Error doing AT*EOBEX (%s)\n", rspbuf);
		goto err;
	}

	return ttyfd;
 err:
	cobex_pe_cleanup(self, TRUE);
	return -1;
}


gint cobex_pe_connect(obex_t *self, gpointer userdata)
{
	cobex_t *c;
        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;

	DEBUG(1, G_GNUC_FUNCTION "() \n");

	if((OBEX_FD(self) = cobex_pe_init(self, c->tty)) < 0)
		return -1;

	return 1;
}

gint cobex_pe_disconnect(obex_t *self, gpointer userdata)
{
	DEBUG(1, G_GNUC_FUNCTION "() \n");
	cobex_pe_cleanup(self, FALSE);
	return 1;
}

/* Called from OBEX-lib when data needs to be written */
gint cobex_pe_write(obex_t *self, gpointer userdata, guint8 *buffer, gint length)
{
	int actual;
	cobex_t *c;
        g_return_val_if_fail (self != NULL, -1);
        g_return_val_if_fail (userdata != NULL, -1);
	c = (cobex_t *) userdata;
	
	DEBUG(1, G_GNUC_FUNCTION "() \n");

	DEBUG(1, G_GNUC_FUNCTION "() Data %d bytes\n", length);

	actual = write(OBEX_FD(self), buffer, length);
	if (actual < length)	{
		g_print("Error writing to port\n");
		return -1;
	}

	return actual;
}

/* Called when input data is needed */
gint cobex_pe_handleinput(obex_t *self, gpointer userdata, gint timeout)
{
	gint ret;
	struct timeval time;
	fd_set fdset;
	guint8 recv[500];

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

	DEBUG(1, G_GNUC_FUNCTION "() There is something (%d)\n", ret);

	/* Check if this is a timeout (0) or error (-1) */
	if(ret <= 0)
		return ret;

	ret = read(OBEX_FD(self), recv, sizeof(recv));
	DEBUG(1, G_GNUC_FUNCTION "() Read %d bytes\n", ret);

	if (ret > 0) {
		OBEX_CustomDataFeed(self, recv, ret);
		return 1;
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
obex_ctrans_t *cobex_pe_ctrans (const gchar *tty) {
	obex_ctrans_t *ctrans;
	cobex_t *cobex;

	cobex = g_new0 (cobex_t, 1);
	if(tty == NULL)
		tty = SERPORT;
	cobex->tty = g_strdup (tty);

	ctrans = g_new0 (obex_ctrans_t, 1);
	ctrans->connect = cobex_pe_connect;
	ctrans->disconnect = cobex_pe_disconnect;
	ctrans->write = cobex_pe_write;
	ctrans->listen = NULL;
	ctrans->handleinput = cobex_pe_handleinput;
	ctrans->userdata = cobex;
	
	return ctrans;
}


void cobex_pe_free (obex_ctrans_t *ctrans)
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
