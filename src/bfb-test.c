/*********************************************************************
 *                
 * Filename:      bfb.c
 * Version:       0.1
 * Description:   Talk BFB over a serial port (Siemens specific)
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

#include <netinet/in.h>
#include "crc.h"

int ttyfd;
char inputbuf[500];
struct termios oldtio, newtio;
guint8 seq;

#define MAX_FRAME_DATA 32
#define BFB_FRAME_CONNECT 0x02
#define BFB_FRAME_INTERFACE 0x01
#define BFB_FRAME_AT 0x06
#define BFB_FRAME_DATA 0x16

#define BFB_DATA_PREPARE 0x01
#define BFB_DATA_FIRST 0x02
#define BFB_DATA_NEXT 0x03

guint8 init_magic = 0x14;
guint8 speed57600[] = {0xc0,'5','7','6','0','0',0x13,0xd2,0x2b}; // bogus?
guint8 speed115200[] = {0xc0,'1','1','5','2','0','0',0x13,0xd2,0x2b};
guint8 sifs[] = {'a','t','^','s','i','f','s',0x13};
guint8 sbfb2[] = {'a','t','^','s','b','f','b','=','0',0x13};

guint8 obex[] = {0x80,0x00,0x1A,0x10,0x00,0x40,0x06,0x46,0x00,0x13,0x6b,0x01,0xCB,0x31,0x41,0x06,0x11,0xD4,0x9A,0x77,0x00,0x50,0xDA,0x3f,0x47,0x1F}; // chk: 0x21 0xE3

/* stuff data frame into serial cable encapsulation */
/* needs to be of len+7 size */
/* Type 0x01: "prepare" command. */
/* Type 0x02: first transmission in a row. */
/* Type 0x03: continued transmission. */
gint data_stuff(guint8 *buffer, guint8 type, guint8 *data, gint len)
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
	buffer[2] = seq++; // maybe someone else should inc this

	fcs.value = htons(len);
	buffer[3] = fcs.bytes[0];
	buffer[4] = fcs.bytes[1];

	// copy data
	memcpy(&buffer[5], data, len);

	// gen CRC
        fcs.value = INIT_FCS;

	g_print(__FUNCTION__ "From (%x) to (%x)\n", buffer[4], buffer[len+4]);
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
gint write_packets(guint8 type, guint8 *buffer, gint length)
{
	guint8 *packet;
	gint i;
	gint l;
	gint actual;
	gint j;

	packet = g_malloc((length > MAX_FRAME_DATA ? MAX_FRAME_DATA : length) + 3);

	for(i=0; i <length; i += MAX_FRAME_DATA) {

		l = length - i;
		if (l > MAX_FRAME_DATA)
			l = MAX_FRAME_DATA;

		packet[0] = type;
		packet[1] = l;
		packet[2] = packet[0] ^ packet[1];

		memcpy(&packet[3], &buffer[i], l);

		actual = write(ttyfd, packet, l + 3);
		g_print(__FUNCTION__ "() Wrote %d bytes (expected %d)\n", actual, l + 3);
		for(j=0; j < actual; j++) {
			g_print("%02x ", packet[j]);
		}
		g_print("\n");
	}
	g_free(packet);
	return i / MAX_FRAME_DATA;
}

/* retrieve actual packets */
gint read_packets(guint8 *buffer, gint length)
{
	gint i;
	gint l;
	guint8 *dest;

	g_print(__FUNCTION__ "()\n");

	if (length < 3) {
		g_print(__FUNCTION__ "() Empty packet?\n");
		return 0;
	}

	dest = buffer;
	for(i=0; i <length; ) {

		l = buffer[i+1];
		g_print(__FUNCTION__ "() Packet %d (%d bytes)\n", buffer[i], buffer[i+1]);
		memmove(dest, &buffer[i+3], l);
		dest += l;

		i += l + 3;
	}
	return dest-buffer;
}


/* retrieve data synch */
gint bfb_sync_read(guint8 *buffer, gint length)
{
	fd_set ttyset;
	struct timeval tv;
	int actual;

	FD_ZERO(&ttyset);
	FD_SET(ttyfd, &ttyset);
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if(select(ttyfd+1, &ttyset, NULL, NULL, &tv))	{
		actual = read(ttyfd, buffer, length);
		if(actual < 0)
			g_print(__FUNCTION__ "() Read error: %d\n", actual);
//	printf("tmpbuf=%d: %s\n", actual, buffer);
//	for(i=0;i<actual;i++) printf("%02x ", buffer[i]);
//	printf("\n");
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
	int actual;

	g_print(__FUNCTION__ "()\n");

	actual = read(ttyfd, inputbuf, sizeof(inputbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);


	inputbuf[actual] = '\0';
	g_print(inputbuf);

	actual = read_packets(inputbuf, actual);

	//OBEX_CustomDataFeed(cobex_handle, inputbuf, actual);
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

/* Set the phone in BFB-mode */
gint cobex_init(char *ttyname)
{
	int actual;

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
		goto err;
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

	actual = write_packets(BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));
	g_print(__FUNCTION__ "() Wrote %d packets\n", actual);
	actual = bfb_sync_read(rspbuf, sizeof(rspbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);
	actual = read_packets(rspbuf, actual);
	g_print(__FUNCTION__ "() Unstuffed %d bytes\n", actual);

	if ((actual != 2) || (rspbuf[0] != init_magic) || (rspbuf[1] != 0xaa)) {
		printf("Error doing BFB init (%d, %x %x)\n", actual, rspbuf[0], rspbuf[1]);
		goto err;
	}


/*
	actual = write_packets(BFB_FRAME_INTERFACE, speed115200, sizeof(speed115200));
	g_print(__FUNCTION__ "() Wrote %d packets\n", actual);
*/

	actual = write_packets(BFB_FRAME_AT, sifs, sizeof(sifs));
	g_print(__FUNCTION__ "() Wrote %d packets\n", actual);

	actual = data_stuff(rspbuf, BFB_DATA_FIRST, obex, sizeof(obex));
	actual = write_packets(BFB_FRAME_DATA, rspbuf, actual);

	actual = bfb_sync_read(rspbuf, sizeof(rspbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);
	actual = read_packets(rspbuf, actual);
	g_print(__FUNCTION__ "() Unstuffed %d bytes\n", actual);



	return 1;
err:
	cobex_cleanup(TRUE);
	return -1;
}

/* Clear the phone out of BFB-mode */
gint cobex_deinit(char *ttyname)
{
	int actual;

	guint8 rspbuf[200];

	printf(__FUNCTION__ "()\n");

	sleep(1); // synch a bit

//	actual = write_packets(BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));

/*
	actual = write_packets(BFB_FRAME_INTERFACE, speed57600, sizeof(speed57600));
	g_print(__FUNCTION__ "() Wrote %d packets\n", actual);
	actual = bfb_sync_read(rspbuf, sizeof(rspbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);
	actual = read_packets(rspbuf, actual);
	g_print(__FUNCTION__ "() Unstuffed %d bytes\n", actual);
*/

	actual = data_stuff(rspbuf, BFB_DATA_PREPARE, 0, 0);
	actual = write_packets(BFB_FRAME_DATA, rspbuf, actual);
	actual = write_packets(BFB_FRAME_AT, sbfb2, sizeof(sbfb2));

	g_print(__FUNCTION__ "() Wrote %d packets\n", actual);
	actual = bfb_sync_read(rspbuf, sizeof(rspbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", actual);
	actual = read_packets(rspbuf, actual);
	g_print(__FUNCTION__ "() Unstuffed %d bytes\n", actual);

/*
	if ((actual != 2) || (rspbuf[0] != init_magic) || (rspbuf[1] != 0xaa)) {
		printf("Error doing BFB init (%d, %x %x)\n", actual, rspbuf[0], rspbuf[1]);
		goto err;
	}
*/

	sleep(1); // synch a bit

	if(cobex_do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		printf("Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0) {
		printf("Error doing ATZ (%s)\n", rspbuf);
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

gint cobex_connect()
{
	printf(__FUNCTION__ "()\n");

	if(cobex_init(SERPORT) < 0)
		return -1;

	seq = 0;
	/*
	if(cobex_start_io() < 0)
		return -1;
	*/
	return 1;
}

gint cobex_disconnect()
{
	printf(__FUNCTION__ "()\n");

	if(cobex_deinit(SERPORT) < 0)
		return -1;

	cobex_cleanup(FALSE);
	return 1;
}

/* Called from OBEX-lib when data needs to be written */
gint cobex_write(guint8 *buffer, gint length)
{
	guint8 magic = 0x14;

	int actual;
	guint8 *stuffed;
	
	printf(__FUNCTION__ "()\n");

	g_print(__FUNCTION__ "() Data %d bytes\n", length);
	stuffed = g_malloc(length + 7);

	if (seq == 0){
		actual = data_stuff(stuffed, 2, buffer, length);
	} else {
		actual = data_stuff(stuffed, 1, NULL, 0);

		g_print(__FUNCTION__ "() Stuffed %d bytes\n", actual);

		actual = write_packets(BFB_FRAME_DATA, stuffed, actual);
		g_print(__FUNCTION__ "() Wrote %d packets\n", actual);

		actual = data_stuff(stuffed, 3, buffer, length);
	}

	g_print(__FUNCTION__ "() Stuffed %d bytes\n", actual);

	actual = write_packets(BFB_FRAME_DATA, stuffed, actual);
	g_print(__FUNCTION__ "() Wrote %d packets (%d bytes)\n", actual, length);

	return actual;
}

/* Called when data arrives */
gint cobex_handleinput(gint timeout)
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

	ret = read(ttyfd, inputbuf, sizeof(inputbuf));
	g_print(__FUNCTION__ "() Read %d bytes\n", ret);

	if (ret > 0) {
		read_packets(inputbuf, ret);
		// OBEX_CustomDataFeed(self, inputbuf, ret);
	}

	return 0;
}

int main(int argc, char *argv[])
{

	cobex_connect();
	cobex_disconnect();
}

// gcc -o bfb crc.o -lglib bfb.c
