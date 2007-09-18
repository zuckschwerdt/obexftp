/**
	\file bfb/bfb_io.c
	BFB transport encapsulation (for Siemens mobile equipment).
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>

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
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#define sleep(n)	Sleep(n*1000)
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "bfb.h"
#include "bfb_io.h"
#include <common.h>

/* Write out an IO buffer */
int bfb_io_write(fd_t fd, const uint8_t *buffer, int length)
{
#ifdef _WIN32
	DWORD bytes;
	DEBUG(3, "%s() WriteFile\n", __func__);
        if (fd == INVALID_HANDLE_VALUE) {
		DEBUG(1, "%s() Error INVALID_HANDLE_VALUE\n", __func__);
		return -1;
	}
	if(!WriteFile(fd, buffer, length, &bytes, NULL))
		DEBUG(1, "%s() Write error: %ld\n", __func__, bytes);
	return bytes;
#else
	int bytes;
	if (fd <= 0) {
		DEBUG(1, "%s() Error file handle invalid\n", __func__);
		return -1;
	}
	bytes = write(fd, buffer, length);
	if (bytes < length)
		DEBUG(1, "%s() Error short write (%d / %d)\n", __func__, bytes, length);
	if (bytes < 0)
		DEBUG(1, "%s() Error writing to port\n", __func__);
	return bytes;
#endif
}

/* Read an answer to an IO buffer of max length bytes */
int bfb_io_read(fd_t fd, uint8_t *buffer, int length, int timeout)
{
#ifdef _WIN32
	DWORD bytes;

	DEBUG(3, "%s() ReadFile\n", __func__);

        return_val_if_fail (fd != INVALID_HANDLE_VALUE, -1);

	if (!ReadFile(fd, buffer, length, &bytes, NULL)) {
		DEBUG(2, "%s() Read error: %ld\n", __func__, bytes);
	}

	return bytes;
#else
	struct timeval time;
	fd_set fdset;
	int actual;

        return_val_if_fail (fd > 0, -1);

	time.tv_sec = timeout;
	time.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	if(select(fd+1, &fdset, NULL, NULL, &time)) {
		actual = read(fd, buffer, length);
		if(actual < 0)
			DEBUG(2, "%s() Read error: %d\n", __func__, actual);
		return actual;
	}
	else {
		DEBUG(1, "%s() No data (timeout: %d)\n", __func__, timeout);
		return 0;
	}
#endif
}

/* Send an BFB init command an check for a valid answer frame */
int bfb_io_init(fd_t fd)
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
		DEBUG(2, "%s() Wrote %d packets\n", __func__, actual);

		if (actual < 1) {
			DEBUG(1, "BFB port error\n");
			return FALSE;
		}

		actual = bfb_io_read(fd, rspbuf, sizeof(rspbuf), 1);
		DEBUG(2, "%s() Read %d bytes\n", __func__, actual);

		if (actual < 1) {
			DEBUG(1, "BFB read error\n");
			return FALSE;
		}

		frame = bfb_read_packets(rspbuf, &actual);
		DEBUG(2, "%s() Unstuffed, %d bytes remaining\n", __func__, actual);

		if (frame != NULL)
			break;
	}
		
	if (frame == NULL) {
		DEBUG(1, "BFB init error\n");
		return FALSE;
	}
	DEBUG(2, "BFB init ok.\n");

	if ((frame->len == 2) && (frame->payload[0] == init_magic) && (frame->payload[1] == init_magic2)) {
		free(frame);
		return TRUE;
	}

	DEBUG(1, "Error doing BFB init (%d, %x %x)\n",
		frame->len, frame->payload[0], frame->payload[1]);

	free(frame);

	return FALSE;
}

/* Send an AT-command an expect 1 line back as answer */
/* Ericsson may choose to answer one line, blank one line and then send OK */
int do_at_cmd(fd_t fd, const char *cmd, char *rspbuf, int rspbuflen)
{
#ifdef _WIN32
	DWORD actual;
#else
	int actual;
#endif

	char *answer = NULL;
	char *answer_end = NULL;
	int answer_size;

	char tmpbuf[100] = {0,};
	int total = 0;
	int done = 0;
	int cmdlen;

        return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	DEBUG(3, "%s() Sending %d: %s\n", __func__, cmdlen, cmd);

	/* Write command */
	if(bfb_io_write(fd, cmd, cmdlen) < cmdlen)
		return -1;

	while(!done)	{
		actual = bfb_io_read(fd, &tmpbuf[total], sizeof(tmpbuf) - total, 2);
		/* error checking */
       		if(actual < 0)
       			return actual;

		if(actual == 0)
			/* Anser didn't come in time. Cancel */
			return -1;
		
       		total += actual;

       		DEBUG(3, "%s() tmpbuf=%d: %s\n", __func__, total, tmpbuf);

       		/* Answer not found within 100 bytes. Cancel */
       		if(total == sizeof(tmpbuf))
       			return -1;

       		if( (answer = strchr(tmpbuf, '\n')) )	{
			while((*answer == '\r') || (*answer == '\n'))
				answer++;
       			/* Remove first line (echo) */
       			if( (answer_end = strchr(answer+1, '\n')) )	{
       				/* Found end of answer */
       				done = 1;
       			}
       		}
	}
	/* try hard to discard remaing lines */
	actual = bfb_io_read(fd, &tmpbuf[total], sizeof(tmpbuf) - total, 2);

/* 	DEBUG(3, "%s() buf:%08lx answer:%08lx end:%08lx\n", __func__, tmpbuf, answer, answer_end); */

	DEBUG(3, "%s() Answer: %s\n", __func__, answer);

	/* Remove heading and trailing \r */
	while((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	DEBUG(3, "%s() Answer: %s\n", __func__, answer);

	answer_size = (answer_end) - answer +1;

	DEBUG(2, "%s() Answer size=%d\n", __func__, answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;

	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = '\0';
	return 0;
}

/* close the connection */
void bfb_io_close(fd_t fd, int force)
{
	DEBUG(3, "%s()\n", __func__);
#ifdef _WIN32
        return_if_fail (fd != INVALID_HANDLE_VALUE);
#else
        return_if_fail (fd > 0);
#endif

/* we don't know if it's a BFB type link, don't send */
/*	bfb_write_at(fd, "at^sbfb=0\r");	*/

	if(force)	{
		/* Send a break to get out of OBEX-mode */
#ifdef _WIN32
		if(SetCommBreak(fd) != TRUE)	{
#elif defined(TCSBRKP)
		if(ioctl(fd, TCSBRKP, 0) < 0) {
#elif defined(TCSBRK)
		if(ioctl(fd, TCSBRK, 0) < 0) {
#else
		if(tcsendbreak(fd, 0) < 0) {
#endif
			DEBUG(1, "Unable to send break!\n");
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
fd_t bfb_io_open(const char *ttyname, enum trans_type *typeinfo)
{
	char rspbuf[200];
#ifdef _WIN32
	HANDLE ttyfd;
	DCB dcb;
	COMMTIMEOUTS ctTimeOuts;

        return_val_if_fail (ttyname != NULL, INVALID_HANDLE_VALUE);

	DEBUG(3, "%s() CreateFile\n", __func__);
	ttyfd = CreateFile (ttyname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ttyfd == INVALID_HANDLE_VALUE) {
		DEBUG(1, "Error: CreateFile()\n");
		return INVALID_HANDLE_VALUE;
	}

	if(!GetCommState(ttyfd, &dcb)) {
		DEBUG(1, "Error: GetCommState()\n");
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
		DEBUG(1, "SetCommState failed\n");

	ctTimeOuts.ReadIntervalTimeout = 250;
	ctTimeOuts.ReadTotalTimeoutMultiplier = 1; /* no good with big buffer */
	ctTimeOuts.ReadTotalTimeoutConstant = 500;
	ctTimeOuts.WriteTotalTimeoutMultiplier = 1;
	ctTimeOuts.WriteTotalTimeoutConstant = 5000;
	if(!SetCommTimeouts(ttyfd, &ctTimeOuts)) {
		DEBUG(1, "Error: SetCommTimeouts()\n");
	}
       
	Sleep(500);

	/* flush all pending input */
	if(!PurgeComm(ttyfd, PURGE_RXABORT | PURGE_RXCLEAR)) {
		DEBUG(1, "Error: PurgeComm\n");
	}

#else
	int ttyfd;
	struct termios oldtio, newtio;

        return_val_if_fail (ttyname != NULL, -1);

	DEBUG(2, "%s() \n", __func__);

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		DEBUG(1, "Can' t open tty\n");
		return -1;
	}

	(void) tcgetattr(ttyfd, &oldtio);
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = B57600 | CS8 | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	(void) tcflush(ttyfd, TCIFLUSH);
	(void) tcsetattr(ttyfd, TCSANOW, &newtio);
#endif

	/* Can't just try that now, as it will break some phones. */
	//if (bfb_io_init (ttyfd)) {
	//	DEBUG(1, "Already in BFB mode.\n");
	//	goto bfbmode;
	//}

	if(do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		DEBUG(1, "Comm-error or already in BFB mode\n");
#ifdef _WIN32
		goto bfbmode;
#else
		newtio.c_cflag = B19200 | CS8 | CREAD;
		(void) tcflush(ttyfd, TCIFLUSH);
		(void) tcsetattr(ttyfd, TCSANOW, &newtio);
		if(do_at_cmd(ttyfd, "ATZ\r\n", rspbuf, sizeof(rspbuf)) < 0) {
			DEBUG(1, "Comm-error or already in BFB mode\n");
			goto bfbmode;
		}
#endif
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(do_at_cmd(ttyfd, "AT+GMI\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	DEBUG(1, "AT+GMI: %s\n", rspbuf);
	if(strncasecmp("ERICSSON", rspbuf, 8) == 0 ||
	   strncasecmp("SONY ERICSSON", rspbuf, 13) == 0) {
	
		DEBUG(1, "Ericsson detected\n");
		goto ericsson;
	}
	if(strncasecmp("MOTOROLA", rspbuf, 8) == 0 || /* is this needed? */
	   strstr(rspbuf, "Motorola")) {
		DEBUG(1, "Motorola detected\n");
		goto motorola;
	}
	if(strncasecmp("SIEMENS", rspbuf, 7) != 0) {
		DEBUG(1, "No Siemens detected. Trying generic.\n");
		goto generic;
	}

	if(do_at_cmd(ttyfd, "AT^SIFS\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0	/* DCA-500, DCA-510 */
	   && strcasecmp("^SIFS: BLUE", rspbuf) != 0
	   && strcasecmp("^SIFS: IRDA", rspbuf) != 0
	   && strcasecmp("^SIFS: USB", rspbuf) != 0) {	/* DCA-540 */
		DEBUG(1, "Unknown connection doing AT^SIFS (%s), continuing anyway ...\n", rspbuf);
	}

	/* prefer connection without BFB */
	if(do_at_cmd(ttyfd, "AT^SBFB=?\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(!strncasecmp("^SBFB: (0-3", rspbuf, 11) != 0)	{
		DEBUG(1, "New plain Siemens protocol. (%s)\n", rspbuf);
		goto newsiemens;
	}
       	DEBUG(1, "Old BFB Siemens protocol. (%s)\n", rspbuf);
	
	if(do_at_cmd(ttyfd, "AT^SBFB=1\r\n", rspbuf, sizeof(rspbuf)) < 0)	{
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT^SBFB=1 (%s)\n", rspbuf);
		goto err;
	}

	sleep(1); /* synch a bit */

 bfbmode:
#ifndef _WIN32
	newtio.c_cflag = B57600 | CS8 | CREAD;
	(void) tcflush(ttyfd, TCIFLUSH);
	(void) tcsetattr(ttyfd, TCSANOW, &newtio);
#endif

	if (! bfb_io_init (ttyfd)) {
		/* well there may be some garbage -- just try again */
		if (! bfb_io_init (ttyfd)) {
			DEBUG(1, "Couldn't init BFB mode.\n");
			goto err;
		}
	}

	*typeinfo = TT_BFB;
	return ttyfd;

 ericsson:
	if(do_at_cmd(ttyfd, "AT*EOBEX\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT*EOBEX (%s)\n", rspbuf);
	       	goto err;
	}
	
	*typeinfo = TT_ERICSSON;
	return ttyfd;

 motorola:
	if(do_at_cmd(ttyfd, "AT+MODE=22\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0 || /* is this needed? */
	   strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT+MODE=22 (%s)\n", rspbuf);
	       	goto err;
	}
	
	*typeinfo = TT_MOTOROLA;
	return ttyfd;

 newsiemens:
	/* ^SBFB=3 only works when switching from RCCP(0) to OBEX(3) 
	 * but the phone may be in GIPSY(2) mode. 
	 * No need to implement BFC(1) mode if we only want to run OBEX.
	 */ 
	if(do_at_cmd(ttyfd, "AT^SQWE?\r\n", rspbuf, sizeof(rspbuf)) < 0) { 
		DEBUG(1, "Comm-error\n"); 
		goto err; 
	} 
	if (strcasecmp("^SQWE:0",rspbuf) != 0) { 
		if(do_at_cmd(ttyfd, "AT^SQWE=0\r\n", rspbuf, sizeof(rspbuf)) < 0) { 
			DEBUG(1, "Comm-error\n"); 
			goto err; 
		} 
		if(strcasecmp("OK", rspbuf) != 0)       { 
			DEBUG(1, "Error doing AT^SQWE=0 (%s)\n", rspbuf); 
			goto err; 
		} 
		sleep(1); 
	} 
	if(do_at_cmd(ttyfd, "AT^SQWE=3\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT^SQWE=3 (%s)\n", rspbuf);
	       	goto err;
	}
	sleep(1); /* synch a bit */
	
	*typeinfo = TT_SIEMENS;
	return ttyfd;

 generic:
	if(do_at_cmd(ttyfd, "AT+CPROT=0\r\n", rspbuf, sizeof(rspbuf)) < 0) {
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT+CPROT=0 (%s)\n", rspbuf);
	       	goto err;
	}
	
	*typeinfo = TT_GENERIC;
	return ttyfd;

 err:
	bfb_io_close(ttyfd, TRUE);
#ifdef _WIN32
	return INVALID_HANDLE_VALUE;
#else
	return -1;
#endif
}

