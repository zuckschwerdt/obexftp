/*
 *  bfb/bfb_io.c: BFB transport encapsulation (for Siemens mobile equipment)
 *
 *  Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */
/*
 *  v0.1:  Don, 25 Jul 2002 03:16:41 +0200
 */

#include <glib.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#define sleep(n)	_sleep(n*1000)
#else /* _WIN32 */
#include <sys/ioctl.h>
#include <termios.h>
#endif /* _WIN32 */

#include "bfb.h"
#include <g_debug.h>

#undef G_LOG_DOMAIN
#define	G_LOG_DOMAIN	BFB_LOG_DOMAIN

/* Write out a BFB buffer */
gint bfb_io_write(FD fd, guint8 *buffer, gint length)
{
#ifdef _WIN32
	DWORD bytes;
	g_debug(G_GNUC_FUNCTION "() WriteFile");
	if(!WriteFile(fd, buffer, length, &bytes, NULL))
		g_info(G_GNUC_FUNCTION "() Write error: %ld", bytes);
	return bytes;
#else
	return write(fd, buffer, length);
#endif
}

/* Read a BFB answer */
gint do_bfb_read(FD fd, guint8 *buffer, gint length)
{
#ifdef _WIN32
	DWORD bytes;

	g_debug(G_GNUC_FUNCTION "() ReadFile");
	if (!ReadFile(fd, buffer, length, &bytes, NULL)) {
		g_info(G_GNUC_FUNCTION "() Read error: %ld", bytes);
	}

	return bytes;
#else
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
#endif
}

/* Send an BFB init command an check for a valid answer frame */
gboolean do_bfb_init(FD fd)
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
gint do_at_cmd(FD fd, char *cmd, char *rspbuf, int rspbuflen)
{
#ifdef _WIN32
	DWORD actual;
#else
	fd_set ttyset;
	struct timeval tv;
	int actual;
#endif

	char *answer;
	char *answer_end = NULL;
	unsigned int answer_size;

	char tmpbuf[100] = {0,};
	int total = 0;
	int done = 0;
	int cmdlen;

        g_return_val_if_fail (fd > 0, -1);
        g_return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	g_debug(G_GNUC_FUNCTION "() Sending %d: %s", cmdlen, cmd);

	// Write command
#ifdef _WIN32
	if(!WriteFile(fd, cmd, cmdlen, &actual, NULL) || (actual != cmdlen)) {
#else
	if(write(fd, cmd, cmdlen) < cmdlen)	{
#endif
		g_warning("Error writing to port");
		return -1;
	}

	while(!done)	{
#ifdef _WIN32
			if (!ReadFile(fd, &tmpbuf[total], sizeof(tmpbuf) - total, &actual, NULL))
				g_info(G_GNUC_FUNCTION "() Read error: %ld", actual);
#else
		FD_ZERO(&ttyset);
		FD_SET(fd, &ttyset);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
			actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
#endif
			if(actual < 0)
				return actual;
			total += actual;

			g_debug(G_GNUC_FUNCTION "() tmpbuf=%d: %s", total, tmpbuf);

			// Answer not found within 100 bytes. Cancel
			if(total == sizeof(tmpbuf))
				return -1;

			if( (answer = strchr(tmpbuf, '\n')) )	{
				// Remove first line (echo)
				if( (answer_end = strchr(answer+1, '\n')) )	{
					// Found end of answer
					done = 1;
				}
			}
#ifndef _WIN32
		}
		else	{
			// Anser didn't come in time. Cancel
			return -1;
		}
#endif
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
void bfb_io_close(FD fd, int force)
{
	g_debug(G_GNUC_FUNCTION "()");
#ifdef _WIN32
        g_return_if_fail (fd != INVALID_HANDLE_VALUE);
#else
        g_return_if_fail (fd > 0);
#endif

	if(force)	{
		// Send a break to get out of OBEX-mode
#ifdef _WIN32
		if(SetCommBreak(fd) != TRUE)	{
#else
		if(ioctl(fd, TCSBRKP, 0) < 0)	{
#endif
			g_warning("Unable to send break!");
		}
		sleep(1);
	}
#ifdef _WIN32
	CloseHandle(fd);
#else
	close(fd);
#endif
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
FD bfb_io_open(const gchar *ttyname)
{
	guint8 rspbuf[200];
#ifdef _WIN32
	HANDLE ttyfd;
	DCB dcb;
	COMMTIMEOUTS ctTimeOuts;

        g_return_val_if_fail (ttyname != NULL, INVALID_HANDLE_VALUE);

	g_debug(G_GNUC_FUNCTION "() CreateFile");
	ttyfd = CreateFile (ttyname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ttyfd == INVALID_HANDLE_VALUE)
		g_error ("CreateFile");

	if(!GetCommState(ttyfd, &dcb))
		g_error ("GetCommState");
	dcb.fBinary = TRUE;
	dcb.BaudRate = CBR_57600;
	dcb.fParity = FALSE;
	dcb.Parity = 0;
	dcb.ByteSize = 8;
	dcb.StopBits = 1;
	dcb.fInX = FALSE;
	dcb.fOutX = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	if(!SetCommState(ttyfd, &dcb))
		g_error ("SetCommState");

	ctTimeOuts.ReadIntervalTimeout = 250;
	ctTimeOuts.ReadTotalTimeoutMultiplier = 1; /* no good with big buffer */
	ctTimeOuts.ReadTotalTimeoutConstant = 500;
	ctTimeOuts.WriteTotalTimeoutMultiplier = 1;
	ctTimeOuts.WriteTotalTimeoutConstant = 5000;
	if(!SetCommTimeouts(ttyfd, &ctTimeOuts))
		g_error ("SetCommTimeouts");
       
	Sleep(500);

	// flush all pending input
	if(!PurgeComm(ttyfd, PURGE_RXABORT | PURGE_RXCLEAR))
		g_error ("PurgeComm");

#else /* _WIN32 */
	gint ttyfd;
	struct termios oldtio, newtio;

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
#endif /* _WIN32 */

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
#ifdef _WIN32
	return INVALID_HANDLE_VALUE;
#else
	return -1;
#endif
}

