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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifndef S_SPLINT_S
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#define sleep(n)	_sleep(n*1000)
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "bfb.h"
#include <common.h>

/* Write out a BFB buffer */
int bfb_io_write(FD fd, uint8_t *buffer, int length)
{
#ifdef _WIN32
	DWORD bytes;
	DEBUG(3, "%s() WriteFile", __func__);
	if(!WriteFile(fd, buffer, length, &bytes, NULL))
		DEBUG(2, "%s() Write error: %ld", __func__, bytes);
	return bytes;
#else
	return write(fd, buffer, length);
#endif
}

/* Read a BFB answer */
int do_bfb_read(FD fd, uint8_t *buffer, int length)
{
#ifdef _WIN32
	DWORD bytes;

	DEBUG(3, "%s() ReadFile", __func__);
	if (!ReadFile(fd, buffer, length, &bytes, NULL)) {
		DEBUG(2, "%s() Read error: %ld", __func__, bytes);
	}

	return bytes;
#else
	struct timeval time;
	fd_set fdset;
	int actual;

        return_val_if_fail (fd > 0, -1);

	time.tv_sec = 1;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	if(select(fd+1, &fdset, NULL, NULL, &time)) {
		actual = read(fd, buffer, length);
		if(actual < 0)
			DEBUG(2, "%s() Read error: %d", __func__, actual);
		return actual;
	}
	else {
		DEBUG(1, "%s() No data", __func__);
		return 0;
	}
#endif
}

/* Send an BFB init command an check for a valid answer frame */
int do_bfb_init(FD fd)
{
	int actual;
	int tries=3;
	bfb_frame_t *frame;
	uint8_t rspbuf[200];

	uint8_t init_magic = BFB_CONNECT_HELLO;
	uint8_t init_magic2 = BFB_CONNECT_HELLO_ACK;
	/*
	uint8_t speed115200[] = {0xc0,'1','1','5','2','0','0',0x13,0xd2,0x2b};
	uint8_t sifs[] = {'a','t','^','s','i','f','s',0x13};
	*/

#ifdef _WIN32
        return_val_if_fail (fd != INVALID_HANDLE_VALUE, FALSE);
#else
        return_val_if_fail (fd > 0, FALSE);
#endif

	while (tries-- > 0) {
		actual = bfb_write_packets (fd, BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));
		DEBUG(2, "%s() Wrote %d packets", __func__, actual);

		if (actual < 1) {
			DEBUG(1, "BFB port error");
			return FALSE;
		}

		actual = do_bfb_read(fd, rspbuf, sizeof(rspbuf));
		DEBUG(2, "%s() Read %d bytes", __func__, actual);

		if (actual < 1) {
			DEBUG(1, "BFB read error");
			return FALSE;
		}

		frame = bfb_read_packets(rspbuf, &actual);
		DEBUG(2, "%s() Unstuffed, %d bytes remaining", __func__, actual);

		if (frame != NULL)
			break;
	}
		
	if (frame == NULL) {
		DEBUG(1, "BFB init error");
		return FALSE;
	}
	DEBUG(2, "BFB init ok.");

	if ((frame->len == 2) && (frame->payload[0] == init_magic) && (frame->payload[1] == init_magic2)) {
		free(frame);
		return TRUE;
	}

	DEBUG(1, "Error doing BFB init (%d, %x %x)",
		frame->len, frame->payload[0], frame->payload[1]);

	free(frame);

	return FALSE;
}

/* Send an AT-command an expect 1 line back as answer */
int do_at_cmd(FD fd, char *cmd, char *rspbuf, int rspbuflen)
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

#ifdef _WIN32
        return_val_if_fail (fd != INVALID_HANDLE_VALUE, -1);
#else
        return_val_if_fail (fd > 0, -1);
#endif
        return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	DEBUG(3, "%s() Sending %d: %s", __func__, cmdlen, cmd);

	// Write command
#ifdef _WIN32
	if(!WriteFile(fd, cmd, cmdlen, &actual, NULL) || (actual != cmdlen)) {
#else
	if(write(fd, cmd, cmdlen) < cmdlen)	{
#endif
		DEBUG(1, "Error writing to port");
		return -1;
	}

	while(!done)	{
#ifdef _WIN32
			if (!ReadFile(fd, &tmpbuf[total], sizeof(tmpbuf) - total, &actual, NULL))
				DEBUG(2, "%s() Read error: %ld", __func__, actual);
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

			DEBUG(3, "%s() tmpbuf=%d: %s", __func__, total, tmpbuf);

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


//	DEBUG(3, "%s() buf:%08lx answer:%08lx end:%08lx", __func__, tmpbuf, answer, answer_end);


	DEBUG(3, "%s() Answer: %s", __func__, answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	DEBUG(3, "%s() Answer: %s", __func__, answer);

	answer_size = (answer_end) - answer +1;

	DEBUG(2, "%s() Answer size=%d", __func__, answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

/* close the connection */
void bfb_io_close(FD fd, int force)
{
	DEBUG(3, "%s()", __func__);
#ifdef _WIN32
        return_if_fail (fd != INVALID_HANDLE_VALUE);
#else
        return_if_fail (fd > 0);
#endif

	if(force)	{
		// Send a break to get out of OBEX-mode
#ifdef _WIN32
		if(SetCommBreak(fd) != TRUE)	{
#else
		if(ioctl(fd, TCSBRKP, 0) < 0)	{
#endif
			DEBUG(1, "Unable to send break!");
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
FD bfb_io_open(const char *ttyname)
{
	uint8_t rspbuf[200];
#ifdef _WIN32
	HANDLE ttyfd;
	DCB dcb;
	COMMTIMEOUTS ctTimeOuts;

        return_val_if_fail (ttyname != NULL, INVALID_HANDLE_VALUE);

	DEBUG(3, "%s() CreateFile", __func__);
	ttyfd = CreateFile (ttyname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ttyfd == INVALID_HANDLE_VALUE) {
		DEBUG(1, "Error: CreateFile()");
		return NULL;
	}

	if(!GetCommState(ttyfd, &dcb)) {
		DEBUG(1, "Error: GetCommState()");
	}
	dcb.fBinary = TRUE;
	dcb.BaudRate = CBR_57600;
	dcb.fParity = FALSE;
	dcb.Parity = NOPARITY;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.fInX = FALSE;
	dcb.fOutX = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	if(!SetCommState(ttyfd, &dcb))
		DEBUG(1, "SetCommState failed");

	ctTimeOuts.ReadIntervalTimeout = 250;
	ctTimeOuts.ReadTotalTimeoutMultiplier = 1; /* no good with big buffer */
	ctTimeOuts.ReadTotalTimeoutConstant = 500;
	ctTimeOuts.WriteTotalTimeoutMultiplier = 1;
	ctTimeOuts.WriteTotalTimeoutConstant = 5000;
	if(!SetCommTimeouts(ttyfd, &ctTimeOuts)) {
		DEBUG(1, "Error: SetCommTimeouts()");
	}
       
	Sleep(500);

	// flush all pending input
	if(!PurgeComm(ttyfd, PURGE_RXABORT | PURGE_RXCLEAR)) {
		DEBUG(1, "Error: PurgeComm");
	}

#else
	int ttyfd;
	struct termios oldtio, newtio;

        return_val_if_fail (ttyname != NULL, -1);

	DEBUG(2, "%s() ", __func__);

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		DEBUG(1, "Can' t open tty");
		return -1;
	}

	(void) tcgetattr(ttyfd, &oldtio);
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = B57600 | CS8 | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	(void) tcflush(ttyfd, TCIFLUSH);
	(void) tcsetattr(ttyfd, TCSANOW, &newtio);
#endif

	/* do we need to handle an error? */
	if (do_bfb_init (ttyfd)) {
		DEBUG(1, "Already in BFB mode.");
		goto bfbmode;
	}

	if(do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		DEBUG(1, "Comm-error or already in BFB mode");
		newtio.c_cflag = B19200 | CS8 | CREAD;
		(void) tcflush(ttyfd, TCIFLUSH);
		(void) tcsetattr(ttyfd, TCSANOW, &newtio);
		if(do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0) {
			DEBUG(1, "Comm-error or already in BFB mode");
			goto err;
		}
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing ATZ (%s)", rspbuf);
		goto err;
	}

	if(do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0)	{ // expect "OK" also!
		DEBUG(1, "Error doing AT^SIFS (%s)", rspbuf);
		goto err;
	}

	if(do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT^SBFB=1 (%s)", rspbuf);
		goto err;
	}

	sleep(1); // synch a bit

	newtio.c_cflag = B57600 | CS8 | CREAD;
	(void) tcflush(ttyfd, TCIFLUSH);
	(void) tcsetattr(ttyfd, TCSANOW, &newtio);

 bfbmode:
	if (! do_bfb_init (ttyfd)) {
		// well there may be some garbage -- just try again
		if (! do_bfb_init (ttyfd)) {
			DEBUG(1, "Couldn't init BFB mode.");
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

