/*********************************************************************
 *                
 * Filename:      cobex_S45.c
 * Version:       0.1
 * Description:   Talk OBEX over a serial port (Siemens specific)
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 * Modified at:   Don, 17 Jan 2002 23:46:52 +0100
 * Modified by:   Christian W. Zuckschwerdt <zany@triq.net>
 *
 * Based on code by Pontus Fuchs <pontus@tactel.se>
 * Original Copyright (c) 1998, 1999, Dag Brattli, All Rights Reserved.
 *      
 *     This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU Lesser General Public
 *     License as published by the Free Software Foundation; either
 *     version 2 of the License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *     Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public
 *     License along with this library; if not, write to the Free Software
 *     Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *     MA  02111-1307  USA
 *     
 ********************************************************************/

#define SERPORT "/dev/ttyS1"

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
#include <openobex/obex.h>
#include "cobex_S45.h"

int ttyfd;
char inputbuf[500];
obex_t *cobex_handle;
struct termios oldtio, newtio;


/* Called when data arrives */
void cobex_input_handler(int signal)
{
	int actual;

	g_print(__FUNCTION__ "()\n");

	actual = read(ttyfd, &inputbuf, sizeof(inputbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);

	OBEX_CustomDataFeed(cobex_handle, inputbuf, actual);
}


/* Send an AT-command an expect 1 line back as answer */
int cobex_do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen)
{
	fd_set ttyset;
	struct timeval tv;

	char *answer;
	char *answer_end;
	unsigned int answer_size;

	char tmpbuf[100] = {0,};
	int actual;
	int total = 0;
	int done = 0;
	int cmdlen;

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
/*	printf("Sending %d: %s\n", cmdlen, cmd); */

	// Write command
	if(write(fd, cmd, cmdlen) < cmdlen)	{
		perror("Error writing to port");
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

//			printf("tmpbuf=%d: %s\n", total, tmpbuf);

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


//	printf("buf:%08lx answer:%08lx end:%08lx\n", tmpbuf, answer, answer_end);


//	printf("Answer: %s\n", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
//	printf("Answer: %s\n", answer);

	answer_size = (answer_end) - answer +1;

//	printf("Answer size=%d\n", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

/* Set the phone in OBEX-mode */
gint cobex_init(char *ttyname)
{
	char rspbuf[200];

	printf(__FUNCTION__ "()\n");

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 )	{
		perror("Can' t open tty");
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
		printf("Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		printf("Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}
/*
	if(cobex_do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		printf("Comm-error\n");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0)	{ // expect "OK" also!
		printf("Error doing AT^SIFS (%s)\n", rspbuf);
		goto err;
	}
*/
	if(cobex_do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		printf("Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		printf("Error doing AT^SBFB=1 (%s)\n", rspbuf);
		goto err;
	}

	return 1;
err:
	cobex_cleanup(TRUE);
	return -1;
}

/* Set up input-handler */
int cobex_start_io(void)
{
	int oflags;
	int ret;

	signal(SIGIO, &cobex_input_handler);
	fcntl(ttyfd, F_SETOWN, getpid());
	oflags = fcntl(0, F_GETFL);
	ret = fcntl(ttyfd, F_SETFL, oflags | FASYNC);
	if(ret < 0)
		return ret;
	return 0;
}

void cobex_cleanup(int force)
{
	signal(SIGIO, SIG_IGN);
	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(ttyfd, TCSBRKP, 0) < 0)	{
			printf("Unable to send break!\n");
		}
		sleep(1);
	}
	close(ttyfd);
	ttyfd = -1;
}


gint cobex_connect(obex_t *self, gpointer userdata)
{
	printf(__FUNCTION__ "()\n");

	cobex_handle = self;

	if(cobex_init(SERPORT) < 0)
		return -1;
	if(cobex_start_io() < 0)
		return -1;
	return 1;
}

gint cobex_disconnect(obex_t *self, gpointer userdata)
{
	printf(__FUNCTION__ "()\n");
	cobex_cleanup(FALSE);
	return 1;
}

/* Called from OBEX-lib when data needs to be written */
gint cobex_write(obex_t *self, gpointer userdata, guint8 *buffer, gint length)
{
	int actual;
	
	printf(__FUNCTION__ "()\n");

	actual = write(ttyfd, buffer, length);
	g_print(__FUNCTION__ "() Wrote %d bytes (expected %d)\n", actual, length);

	return actual;
}

/* Called when data arrives */
gint cobex_handleinput(obex_t *self, gpointer userdata, gint timeout)
{
	int ret;
	struct timeval time;
	fd_set fdset;

	g_print(__FUNCTION__ "()\n");

	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(ttyfd, &fdset);

	/* Wait for input */
	ret = select(ttyfd + 1, &fdset, NULL, NULL, &time);

	g_print(__FUNCTION__ "() There is something (%d)\n", ret);

	/* Check if this is a timeout (0) or error (-1) */
	if (ret < 1)
		return ret;

	ret = read(ttyfd, &inputbuf, sizeof(inputbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", ret);

	OBEX_CustomDataFeed(self, inputbuf, ret);

	return 0;
}

obex_ctrans_t custfunc;
/* Maybe this could be defined in the header file? */
obex_ctrans_t *cobex_ctrans(void)
{
	custfunc.connect = cobex_connect;
	custfunc.disconnect = cobex_disconnect;
	custfunc.write = cobex_write;
	custfunc.listen = NULL;
	custfunc.handleinput = cobex_handleinput;
	return &custfunc;
}
