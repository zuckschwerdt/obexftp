/*********************************************************************
 *                
 * Filename:      cobex_S45.c
 * Version:       0.1
 * Description:   Talk OBEX over a serial port (Siemens specific)
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 * Modified at:   Don,  7 Feb 2002 12:24:55 +0100
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
#include "debug.h"


obex_t *cobex_handle;

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
			g_print(__FUNCTION__ "() Read error: %d\n", actual);
		return actual;
	}
	else {
		g_print(__FUNCTION__ "() No data\n");
		return 0;
	}
}


/* Called when data arrives */
void cobex_input_handler(int signal)
{
	cobex_t *c = (cobex_t *)OBEX_GetUserData(cobex_handle);
	int actual;
	gint i;

	g_print(__FUNCTION__ "()\n");

	g_print(__FUNCTION__ "() Buffer size %d\n", c->recv_len);

	actual = read(c->fd, &(c->recv[c->recv_len]), sizeof(c->recv)-c->recv_len);
	c->recv_len += actual;
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);

	for(i = 0; i < c->recv_len; i++) {
		g_print("%02x ", c->recv[i]);
	}
	g_print("\n");

/*
	actual = bfb_read_packets(c->recv, actual);

	OBEX_CustomDataFeed(cobex_handle, inputbuf, actual);
*/
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

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
gint cobex_init(char *ttyname)
{
	int ttyfd;
	struct termios oldtio, newtio;

	int actual;
	bfb_frame_t *frame;
	guint8 init_magic = 0x14;
	guint8 init_magic2 = 0xaa;
	guint8 speed[] = {0xc0,'1','1','5','2','0','0',0x13,0xd2,0x2b};
	guint8 sifs[] = {'a','t','^','s','i','f','s',0x13};

	guint8 rspbuf[200];

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
		goto bfbmode;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		printf("Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		printf("Comm-error\n");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0)	{ // expect "OK" also!
		printf("Error doing AT^SIFS (%s)\n", rspbuf);
		goto err;
	}

	if(cobex_do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		printf("Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		printf("Error doing AT^SBFB=1 (%s)\n", rspbuf);
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
		printf("Error doing BFB init (%d, %x %x)\n",
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

/* Set up input-handler */
int cobex_start_io(void)
{
	cobex_t *c = (cobex_t *)OBEX_GetUserData(cobex_handle);
	int oflags;
	int ret;

	signal(SIGIO, &cobex_input_handler);
	fcntl(c->fd, F_SETOWN, getpid());
	oflags = fcntl(0, F_GETFL);
	ret = fcntl(c->fd, F_SETFL, oflags | FASYNC);
	if(ret < 0)
		return ret;
	return 0;
}

void cobex_cleanup(int force)
{
	cobex_t *c = (cobex_t *)OBEX_GetUserData(cobex_handle);
	signal(SIGIO, SIG_IGN);
	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(c->fd, TCSBRKP, 0) < 0)	{
			printf("Unable to send break!\n");
		}
		sleep(1);
	}
	close(c->fd);
	c->fd = -1;
}


gint cobex_connect(obex_t *self, gpointer userdata)
{
	cobex_t *c;

	printf(__FUNCTION__ "()\n");

	cobex_handle = self;

	c = g_new0(cobex_t, 1);
	OBEX_SetUserData(self, c);

	if((c->fd = cobex_init(SERPORT)) < 0)
		return -1;
/*
	if(cobex_start_io() < 0)
		return -1;
*/
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
	cobex_t *c = (cobex_t *)OBEX_GetUserData(self);

	int actual;
	
	DEBUG(1, G_GNUC_FUNCTION  "() \n");

	DEBUG(1, G_GNUC_FUNCTION  "() Data %d bytes\n", length);


	sleep(1);

	if (c->seq == 0){
		actual = bfb_send_data(c->fd, BFB_DATA_FIRST, buffer, length, c->seq++);
		DEBUG(2, G_GNUC_FUNCTION  "() Wrote %d first packets (%d bytes)\n", actual, length);
	} else {
		actual = bfb_send_data(c->fd, BFB_DATA_PREPARE, NULL, 0, 0);
		DEBUG(2, G_GNUC_FUNCTION  "() Wrote %d prepare packet\n", actual);

		actual = bfb_send_data(c->fd, BFB_DATA_NEXT, buffer, length, c->seq++);
		DEBUG(2, G_GNUC_FUNCTION  "() Wrote %d packets (%d bytes)\n", actual, length);
	}


	return actual;
}

/* Called when input data is needed */
gint cobex_handleinput(obex_t *self, gpointer userdata, gint timeout)
{
	cobex_t *c = (cobex_t *)OBEX_GetUserData(self);
	int ret;
	struct timeval time;
	fd_set fdset;
	bfb_frame_t *frame;

	time.tv_sec = timeout;
	time.tv_usec = 0;

	/* Add the fd's to the set. */
	FD_ZERO(&fdset);
	FD_SET(c->fd, &fdset);

	/* Wait for input */
	ret = select(c->fd + 1, &fdset, NULL, NULL, &time);

	g_print(__FUNCTION__ "() There is something (%d)\n", ret);

	/* Check if this is a timeout (0) or error (-1) */
	if(ret < 1)
		return ret;

	ret = read(c->fd, &(c->recv[c->recv_len]), sizeof(c->recv) - c->recv_len);
	g_print(__FUNCTION__ "() Read %d bytes (%d bytes already buffered)\n", ret, c->recv_len);

	if (ret > 0) {
		c->recv_len += ret;

		while ((frame = bfb_read_packets(c->recv, &(c->recv_len)))) {
			g_print(__FUNCTION__ "() Parsed %x (%d bytes remaining)\n", frame->type, c->recv_len);

			c->data = bfb_assemble_data(c->data, &(c->data_len), frame);

			if (bfb_check_data(c->data, c->data_len) == 1) {
				OBEX_CustomDataFeed(self, &(c->data->data), c->data_len-7);
				g_free(c->data);
				c->data = NULL;
				c->data_len = 0;
				return 1;
			}
		}

	}
	return ret; // 0 or -1
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
