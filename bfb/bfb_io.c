/*
 * bfb_io.c - BFB transport encapsulation (used for Siemens mobile equipment)
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
 * v0.1:  Don, 25 Jul 2002 03:16:41 +0200
 */

#include <glib.h>

#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "bfb.h"
#include <g_debug.h>

#undef G_LOG_DOMAIN
#define	G_LOG_DOMAIN	BFB_LOG_DOMAIN

/* Read a BFB answer */
gint do_bfb_read(int fd, guint8 *buffer, gint length)
{
	struct timeval time;
	fd_set fdset;
	int actual;

        g_return_val_if_fail (fd > 0, -1);

	time.tv_sec = 1;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	if(select(fd+1, &fdset, NULL, NULL, &time)) {
		actual = read(fd, buffer, length);
		if(actual < 0)
			g_info(G_GNUC_FUNCTION "() Read error: %d", actual);
		return actual;
	}
	else {
		g_warning(G_GNUC_FUNCTION "() No data");
		return 0;
	}
}

/* Send an BFB init command an check for a valid answer frame */
gboolean do_bfb_init(int fd)
{
	gint actual;
	bfb_frame_t *frame;
	guint8 rspbuf[200];

	guint8 init_magic = BFB_CONNECT_HELLO;
	guint8 init_magic2 = BFB_CONNECT_HELLO_ACK;
	/*
	guint8 speed115200[] = {0xc0,'1','1','5','2','0','0',0x13,0xd2,0x2b};
	guint8 sifs[] = {'a','t','^','s','i','f','s',0x13};
	*/

        g_return_val_if_fail (fd > 0, FALSE);

	actual = bfb_write_packets (fd, BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));
	g_info(G_GNUC_FUNCTION "() Wrote %d packets", actual);

	if (actual < 1) {
		g_warning("BFB port error");
		return FALSE;
	}

	actual = do_bfb_read(fd, rspbuf, sizeof(rspbuf));
	g_info(G_GNUC_FUNCTION  "() Read %d bytes", actual);

	if (actual < 1) {
		g_warning("BFB read error");
		return FALSE;
	}

	frame = bfb_read_packets(rspbuf, &actual);
	g_info(G_GNUC_FUNCTION  "() Unstuffed, %d bytes remaining", actual);
	if (frame == NULL) {
		g_warning("BFB init error");
		return FALSE;
	}
	g_message("BFB init ok");

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
gint do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen)
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
	g_debug(G_GNUC_FUNCTION "() Sending %d: %s", cmdlen, cmd);

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

			g_debug(G_GNUC_FUNCTION "() tmpbuf=%d: %s", total, tmpbuf);

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


//	g_debug(G_GNUC_FUNCTION "() buf:%08lx answer:%08lx end:%08lx", tmpbuf, answer, answer_end);


	g_debug(G_GNUC_FUNCTION "() Answer: %s", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	g_debug(G_GNUC_FUNCTION "() Answer: %s", answer);

	answer_size = (answer_end) - answer +1;

	g_info(G_GNUC_FUNCTION "() Answer size=%d", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

/* close the connection */
void bfb_io_close(int fd, int force)
{
        g_return_if_fail (fd > 0);

	if(force)	{
		// Send a break to get out of OBEX-mode
		if(ioctl(fd, TCSBRKP, 0) < 0)	{
			g_warning("Unable to send break!");
		}
		sleep(1);
	}
	close(fd);
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
gint bfb_io_open(const gchar *ttyname)
{
	gint ttyfd;
	struct termios oldtio, newtio;
	guint8 rspbuf[200];

        g_return_val_if_fail (ttyname != NULL, -1);

	g_info(G_GNUC_FUNCTION "() ");

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
	if (do_bfb_init (ttyfd)) {
		g_warning("Already in BFB mode.");
		goto bfbmode;
	}

	if(do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_warning("Comm-error or already in BFB mode");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_warning("Error doing ATZ (%s)", rspbuf);
		goto err;
	}

	if(do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_warning("Comm-error");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0)	{ // expect "OK" also!
		g_warning("Error doing AT^SIFS (%s)", rspbuf);
		goto err;
	}

	if(do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		g_warning("Comm-error");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		g_warning("Error doing AT^SBFB=1 (%s)", rspbuf);
		goto err;
	}

	sleep(1); // synch a bit
 bfbmode:
	if (! do_bfb_init (ttyfd)) {
		// well there may be some garbage -- just try again
		if (! do_bfb_init (ttyfd)) {
			g_warning("Couldn't init BFB mode.");
			goto err;
		}
	}

	return ttyfd;
 err:
	bfb_io_close(ttyfd, TRUE);
	return -1;
}

