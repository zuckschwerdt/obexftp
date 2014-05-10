/**
	\file bfb/bfb_io.c
	Detect, initiate and run OBEX over custom serial port protocols
	(Siemens, Ericsson, New-Siemens, Motorola, Generic).
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
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(n)	Sleep(n*1000)
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "multi_cobex_io.h"
#include "bfb/bfb.h"
#include "bfb/bfb_io.h"
#include <common.h>

/* Write out an IO buffer */
int cobex_io_write(fd_t fd, const void *buffer, int length)
{
	struct timeval timeout;
	fd_set fds;

        if (fd == INVALID_HANDLE_VALUE) {
		DEBUG(1, "%s() Error file handle invalid\n", __func__);
		return -1;
	}

	/* select setup */
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	/* Set time limit. */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	if ( select(fd+1, NULL, &fds, NULL, &timeout) > 0) {
#ifdef _WIN32
		DWORD bytes;
		DEBUG(3, "%s() WriteFile\n", __func__);
		if(!WriteFile(fd, buffer, length, &bytes, NULL))
			DEBUG(1, "%s() Write error: %ld\n", __func__, bytes);
		return bytes;
#else
		int bytes;
		bytes = write(fd, buffer, length);
		if (bytes < length)
			DEBUG(1, "%s() Error short write (%d / %d)\n", __func__, bytes, length);
		if (bytes < 0)
			DEBUG(1, "%s() Error writing to port\n", __func__);
		return bytes;
#endif
	} else { /* ! rc > 0*/
		DEBUG(1, "%s() Select failed\n", __func__);
		return 0;
	}
}

/* Read an answer to an IO buffer of max length bytes */
int cobex_io_read(fd_t fd, void *buffer, int length, int timeout)
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

	if (select(fd+1, &fdset, NULL, NULL, &time) > 0) {
		actual = read(fd, buffer, length);
		if (actual < 0) {
			DEBUG(2, "%s() Read error: %d\n", __func__, actual);
		}
		return actual;

	} else {
		DEBUG(1, "%s() No data (timeout: %d)\n", __func__, timeout);
		return 0;
	}
#endif
}

/**
	Read (repeatedly) from fd until a timeout or an error is encountered.
 */
static int cobex_io_read_all(int fd, char *buffer, int length, int timeout)
{
	int actual;
	int pos = 0;
	for (;;) {
		actual = cobex_io_read(fd, &buffer[pos], length - pos, timeout);
		if (actual < 0) return actual;
		if (actual == 0) return pos;
		pos += actual;
	}
}

/**
	Send an AT-command and expect an answer of one or more lines.

	\note Start your command with "AT" and terminate it with "\r" (CR).

	\note The expected lines are the the echo,
		one optional information response and
		a final result code of "OK" or "ERROR".
 */
static bool do_at_cmd(fd_t fd, const char *cmd, char *rspbuf, int rspbuflen)
{
#ifdef _WIN32
	DWORD actual;
#else
	int actual;
#endif

	char *answer = NULL;
	int answer_size;

	char tmpbuf[100] = {0,};
	unsigned int total = 0;
	int cmdlen;

        return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	DEBUG(3, "%s() Sending %d: %s\n", __func__, cmdlen, cmd);

	/* Write command */
	if (cobex_io_write(fd, cmd, cmdlen) < cmdlen)
		return false;

	actual = 1;
	while (actual > 0)	{
		actual = cobex_io_read(fd, &tmpbuf[total], sizeof(tmpbuf) - total, 2);
       		if (actual < 0) /* error checking */
       			return false;
		
       		total += actual;
       		tmpbuf[total] = '\0'; /* terminate the string, always */

       		DEBUG(3, "%s() tmpbuf=%d: %s\n", __func__, total, tmpbuf);

       		/* Answer not found within 100 bytes. Cancel */
       		if (total >= sizeof(tmpbuf))
       			return false;

		/* Remove first line (echo), then search final result code */
		for (answer = tmpbuf; answer && *answer ; ) {
			while ((*answer != '\r') && (*answer != '\n') && (*answer != '\0'))
				answer++;
			while ((*answer == '\r') || (*answer == '\n'))
				answer++;
       			if (!strncmp(answer, "OK\r", 3) || !strncmp(answer, "ERROR\r", 6) ||
			    !strncmp(answer, "OK\n", 3) || !strncmp(answer, "ERROR\n", 6)) {
       				actual = 0; /* we are done */
       			}
       		}
	}
	/* try hard to discard remaing CR/LF */
	if (total > 0 && tmpbuf[total-1] != '\n')
		actual = cobex_io_read(fd, &tmpbuf[total], sizeof(tmpbuf) - total, 2);

	/* Remove echo and trailing CRs */
	answer = strchr(tmpbuf, '\r');
	if (!answer) /* no echo found */
       		return false;

	while (*answer == '\r' || *answer == '\n')
		answer++;
	answer_size = 0;
	while (answer[answer_size] != '\0' &&
		answer[answer_size] != '\r' && answer[answer_size] != '\n')
	answer_size++;

	DEBUG(3, "%s() Answer (size=%d): %s\n", __func__, answer_size, answer);
	if( (answer_size) >= rspbuflen )
		return false;

	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = '\0';
	return true;
}

static void cobex_io_close_bfb(fd_t fd)
{
	/* needed to leave transparent OBEX(3) mode 
	 * and enter RCCP(0) mode. 
	 * DCA-540 needs cable replug, probably problem 
	 * with the linux driver (linux-2.6.17.13) 
	 */
	bfb_write_at(fd, "AT^SBFB=0\r");
	sleep(1);
	cobex_io_write(fd, "+++", 3);
	sleep(1);
	cobex_io_write(fd,"\r", 1);
}

static void cobex_io_close_generic(fd_t fd, int force)
{
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
}

/* close the connection */
void cobex_io_close(fd_t fd, enum cobex_type typeinfo, int force)
{
	DEBUG(3, "%s()\n", __func__);
#ifdef _WIN32
        return_if_fail (fd != INVALID_HANDLE_VALUE);
#else
        return_if_fail (fd > 0);
#endif

	switch(typeinfo)
	{
	case CT_BFB:
		cobex_io_close_bfb(fd);
		break;

	default:
		cobex_io_close_generic(fd, force);
		break;
	}

#ifdef _WIN32
	CloseHandle(fd);
#else
	close(fd);
#endif
}


static fd_t cobex_io_open_port(const char *ttyname)
{
	fd_t ttyfd;

#ifdef _WIN32
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
	struct termios oldtio, newtio;

        return_val_if_fail (ttyname != NULL, INVALID_HANDLE_VALUE);

	DEBUG(2, "%s() \n", __func__);

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		DEBUG(1, "Can' t open tty\n");
		return INVALID_HANDLE_VALUE;
	}

	(void) tcgetattr(ttyfd, &oldtio);
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = B57600 | CS8 | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	(void) tcflush(ttyfd, TCIFLUSH);
	(void) tcsetattr(ttyfd, TCSANOW, &newtio);
#endif

	return ttyfd;
}

static bool cobex_io_init_bfb(fd_t ttyfd)
{
#ifndef _WIN32
	struct termios newtio;
	(void) tcgetattr(ttyfd, &newtio);
	if (newtio.c_cflag != (B57600 | CS8 | CREAD))
	{
		newtio.c_cflag = B57600 | CS8 | CREAD;
		(void) tcsetattr(ttyfd, TCSANOW, &newtio);
	}
	(void) tcflush(ttyfd, TCIFLUSH);
#endif

	if (! bfb_io_init (ttyfd)) {
		/* well there may be some garbage -- just try again */
		if (! bfb_io_init (ttyfd)) {
			DEBUG(1, "Couldn't init BFB mode.\n");
			return false;
		}
	}
	return true;
}

static bool cobex_io_init_siemens2(fd_t ttyfd)
{
	char rspbuf[200];

	/* ^SBFB=3 only works when switching from RCCP(0) to OBEX(3) 
	 * but the phone may be in GIPSY(2) mode. 
	 * No need to implement BFC(1) mode if we only want to run OBEX.
	 */ 
	if(!do_at_cmd(ttyfd, "AT^SQWE?\r", rspbuf, sizeof(rspbuf))) { 
		DEBUG(1, "Comm-error\n"); 
		return false;
	} 
	if (strcasecmp("^SQWE: 0",rspbuf) != 0) {
		if(!do_at_cmd(ttyfd, "AT^SQWE=0\r", rspbuf, sizeof(rspbuf))) { 
			DEBUG(1, "Comm-error\n"); 
			return false;
		} 
		if(strcasecmp("OK", rspbuf) != 0)       { 
			DEBUG(1, "Error doing AT^SQWE=0 (%s)\n", rspbuf); 
			return false;
		} 
		sleep(1); 
	} 
	if(!do_at_cmd(ttyfd, "AT^SQWE=3\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		return false;
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT^SQWE=3 (%s)\n", rspbuf);
		return false;
	}
	sleep(1); /* synch a bit */
	return true;
}

static bool cobex_io_init_siemens(fd_t ttyfd, enum cobex_type *typeinfo)
{
	char rspbuf[200];

	if(!do_at_cmd(ttyfd, "AT^SIFS\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		return false;
	}
	if(strcasecmp("^SIFS: WIRE", rspbuf) != 0	/* DCA-500, DCA-510 */
	   && strcasecmp("^SIFS: BLUE", rspbuf) != 0
	   && strcasecmp("^SIFS: IRDA", rspbuf) != 0
	   && strcasecmp("^SIFS: USB", rspbuf) != 0) {	/* DCA-540 */
		DEBUG(1, "Unknown connection doing AT^SIFS (%s), continuing anyway ...\n", rspbuf);
	}

	/* prefer connection without BFB */
	if(!do_at_cmd(ttyfd, "AT^SBFB=?\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		return false;
	}

	if(!strncasecmp("^SBFB: (0-3", rspbuf, 11) != 0) {
		bool success;
		DEBUG(1, "New plain Siemens protocol. (%s)\n", rspbuf);
		success = cobex_io_init_siemens2(ttyfd);
		if (success)
			*typeinfo = CT_SIEMENS;
		return success;

	} else {
		bool success;
		DEBUG(1, "Old BFB Siemens protocol. (%s)\n", rspbuf);
	
		if(!do_at_cmd(ttyfd, "AT^SBFB=1\r", rspbuf, sizeof(rspbuf))) {
			DEBUG(1, "Comm-error\n");
			return false;
		}
		if(strcasecmp("OK", rspbuf) != 0)	{
			DEBUG(1, "Error doing AT^SBFB=1 (%s)\n", rspbuf);
			return false;
		}

		sleep(1); /* synch a bit */
		success = cobex_io_init_bfb(ttyfd);
		if (success)
			*typeinfo = CT_BFB;
		return success;
	}
}

static bool cobex_io_init_ericsson(fd_t ttyfd)
{
	char rspbuf[200];

	if(!do_at_cmd(ttyfd, "AT*EOBEX\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		return false;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT*EOBEX (%s)\n", rspbuf);
	       	return false;
	}
	return true;
}

static bool cobex_io_init_motorola(fd_t ttyfd)
{
	char rspbuf[200];

	if(!do_at_cmd(ttyfd, "AT+MODE=22\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		return false;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0 || /* is this needed? */
	   strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT+MODE=22 (%s)\n", rspbuf);
	       	return false;
	}
	return true;
}

static bool cobex_io_init_generic(fd_t ttyfd)
{
	char rspbuf[200];

	if(!do_at_cmd(ttyfd, "AT+CPROT=0\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
	       	return false;
	}
	if(strcasecmp("CONNECT", rspbuf) != 0)	{
		DEBUG(1, "Error doing AT+CPROT=0 (%s)\n", rspbuf);
	       	return false;
	}
	return true;
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or INVALID_HANDLE_VALUE on failure */
fd_t cobex_io_open(const char *ttyname, enum cobex_type *typeinfo)
{
	char rspbuf[200];
	int actual;
	fd_t ttyfd = cobex_io_open_port(ttyname);
	const uint8_t transObexCheck[] = {
		0xFF, 0x00, 0x08, 0xCB, 'A', 'T', 'Z', '\r'
	};

	/* Can't just try that now, as it will break some phones. */
	//if (bfb_io_init (ttyfd)) {
	//	DEBUG(1, "Already in BFB mode.\n");
	//	goto bfbmode;
	//}

	/* check if we are in transparent OBEX or AT mode:	*/
	/* send an ABORT (0xFF) with cleverly embedded AT command.	*/
	/* look for valid OBEX frame (OK=0xA0, BADREQ=0xC0, or alike)	*/
	DEBUG(1, "Checking for transparent OBEX mode\n");
	actual = cobex_io_write(ttyfd, transObexCheck, sizeof(transObexCheck));
	if (actual == 8) {
		int i;
		DEBUG(3, "Write ok, reading back\n");
		actual = cobex_io_read_all(ttyfd, rspbuf, sizeof(rspbuf), 2);
		DEBUG(3, "Read %d bytes\n", actual);
		for (i = 0; i < actual; ++i)
		{
			DEBUG(3, "0x%02X %c\n", (uint8_t)rspbuf[i], isprint((int)rspbuf[i])? rspbuf[i]: ' ');
		}
		/* First check for an AT echo, or only for an OK response if echo is disabled by default */
		if (actual >= 3 &&
		    (!strncmp(rspbuf, "ATZ\r", 4) || !strncmp(rspbuf, "\r\nOK\r\n", 4)))
		{
			DEBUG(1, "AT mode\n");
		}
		else if (actual >= 3) {
			DEBUG(3, "Received %02X OBEX frame\n", (uint8_t)rspbuf[0]);
			DEBUG(1, "Transparent OBEX\n");
			*typeinfo = CT_GENERIC;
			return ttyfd;
		}
	}

	if(!do_at_cmd(ttyfd, "ATZ\r", rspbuf, sizeof(rspbuf))) {
#ifdef _WIN32
		DEBUG(1, "Comm-error or already in BFB mode\n");
		goto bfbmode;
#else
		struct termios newtio;
		(void) tcgetattr(ttyfd, &newtio);
		if (newtio.c_cflag != (B19200 | CS8 | CREAD))
		{
			newtio.c_cflag = B19200 | CS8 | CREAD;
			(void) tcsetattr(ttyfd, TCSANOW, &newtio);
		}
		(void) tcflush(ttyfd, TCIFLUSH);

		if(!do_at_cmd(ttyfd, "ATZ\r", rspbuf, sizeof(rspbuf))) {
			DEBUG(1, "Comm-error or already in BFB mode\n");
			goto bfbmode;
		}
#endif
	}
	if(strcasecmp("OK", rspbuf) != 0)	{
		DEBUG(1, "Error doing ATZ (%s)\n", rspbuf);
		goto err;
	}

	if(!do_at_cmd(ttyfd, "AT+GMI\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	DEBUG(1, "AT+GMI: %s\n", rspbuf);

	if(strncasecmp("ERICSSON", rspbuf, 8) == 0 ||
	   strncasecmp("SONY ERICSSON", rspbuf, 13) == 0) {
	
		DEBUG(1, "Ericsson detected\n");
		goto ericsson;

	} else if(strncasecmp("MOTOROLA", rspbuf, 8) == 0 || /* is this needed? */
		  strstr(rspbuf, "Motorola")) {
		DEBUG(1, "Motorola detected\n");
		goto motorola;

	} else if(strncasecmp("SIEMENS", rspbuf, 7) == 0) {
		DEBUG(1, "Siemens detected\n");
		goto siemens;
	}

	// Check for some other devices (mainly Gigasets show this behaviour)
	if(!do_at_cmd(ttyfd, "AT+CGMI\r", rspbuf, sizeof(rspbuf))) {
		DEBUG(1, "Comm-error\n");
		goto err;
	}
	DEBUG(1, "AT+CGMI: %s\n", rspbuf);
	if(strncasecmp("SIEMENS", rspbuf, 7)) {
		DEBUG(1, "Siemens/Gigaset detected\n");
		goto newsiemens;
	}

	DEBUG(1, "Unknown Device detected. Trying generic.\n");
	goto generic;

 siemens:
	if (!cobex_io_init_siemens(ttyfd, typeinfo))
		goto err;
	return ttyfd;

 bfbmode:
	if (!cobex_io_init_bfb(ttyfd))
		goto err;
	*typeinfo = CT_BFB;
	return ttyfd;

 newsiemens:
	if (!cobex_io_init_siemens2(ttyfd))
		goto err;
	*typeinfo = CT_SIEMENS;
	return ttyfd;

 ericsson:
	if (!cobex_io_init_ericsson(ttyfd))
		goto err;
	*typeinfo = CT_ERICSSON;
	return ttyfd;

 motorola:
	if (!cobex_io_init_motorola(ttyfd))
		goto err;
	*typeinfo = CT_MOTOROLA;
	return ttyfd;

 generic:
	if (!cobex_io_init_generic(ttyfd))
		goto err;
	*typeinfo = CT_GENERIC;
	return ttyfd;

 err:
	cobex_io_close(ttyfd, CT_GENERIC, TRUE);
	return INVALID_HANDLE_VALUE;
}
