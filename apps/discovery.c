/**
	\file apps/discovery.c
	Discover mobile and detect its class (Siemens, Ericsson).
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#define speed_t DWORD
typedef HANDLE fd_t;
#else
#include <termios.h>
#include <sys/ioctl.h>
typedef int fd_t;
#endif

#include <common.h>

struct mobile_param {
	const char *gmi;	/* Manufacturer Identification to match */
	const char *proto;	/* Serial Protocol to use */
};

static struct mobile_param mobile_param[] = {
	{"siemens",	"plain"},
	{"ericsson",	"bfb"},
	{0, 0}
};

struct mobile_info {
	char *gmi;	/* AT+GMI : SIEMENS */
	char *gmm;	/* AT+GMM : Gipsy Soft Protocolstack */
	char *gmr;	/* AT+GMR : V2.550 */
	char *cgmi;	/* AT+CGMI : SIEMENS */
	char *cgmm;	/* AT+CGMM : S45 */
	char *cgmr;	/* AT+CGMR : 21 */
	const char *ttyname;
	speed_t speed;
};

/* S45: (auto baud)
   SIEMENS, Gipsy Soft Protocolstack, V2.550
   SIEMENS, S45, 21
 */
/* S25: (fixed at 19200)
   SIEMENS, Gipsy Soft Protocolstack, V2.400
   SIEMENS, S25, 42
 */
/* T68:
   Ericsson, T68, R2E006      prgCXC125326_TAE
   ERICSSON, 1130202-BVT68, R2E006      CXC125319
 */

/* if auto-baud is enabled this will yoield the highest speed first */
#ifdef _WIN32
speed_t speeds[] = {CBR_115200, CBR_57600, CBR_38400, CBR_19200, CBR_9600, 0};
#else
speed_t speeds[] = {B115200, B57600, B38400, B19200, B9600, B0};
#endif

/* Send an AT-command an expect 1 line back as answer */
static int do_at_cmd(fd_t fd, const char *cmd, char *rspbuf, int rspbuflen)
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
	int answer_size;

	char tmpbuf[200] = {0,};
	int total = 0;
	int done = 0;
	int cmdlen;

        return_val_if_fail (fd > 0, -1);
        return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	DEBUG(3, "%s() Sending %d: %s\n", __func__, cmdlen, cmd);

	/* Write command */
#ifdef _WIN32
	if(!WriteFile(fd, cmd, cmdlen, &actual, NULL) || (actual != cmdlen)) {
#else
	if(write(fd, cmd, cmdlen) != cmdlen)	{
#endif
		DEBUG(1, "Error writing to port\n");
		return -1;
	}

	while(!done)	{
#ifdef _WIN32
		if (!ReadFile(fd, &tmpbuf[total], sizeof(tmpbuf) - total, &actual, NULL)) {
				DEBUG(2, "%s() Read error: %ld\n", __func__, actual);
		}
#else
		FD_ZERO(&ttyset);
		FD_SET(fd, &ttyset);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
			sleep(1);
			actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
#endif
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
#ifndef _WIN32
		}
		else	{
			/* Anser didn't come in time. Cancel */
			return -1;
		}
#endif
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

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
static struct mobile_info *probe_tty(const char *ttyname)
{
	int speed;
	uint8_t rspbuf[200];
	struct mobile_info *info;
	char *p;
#ifdef _WIN32
	HANDLE ttyfd;
	DCB dcb;
	COMMTIMEOUTS ctTimeOuts;

        return_val_if_fail (ttyname != NULL, INVALID_HANDLE_VALUE);

	DEBUG(3, "%s() CreateFile\n", __func__);
	ttyfd = CreateFile (ttyname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ttyfd == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Error: CreateFile()\n");
		return NULL;
	}

	if(!GetCommState(ttyfd, &dcb))
		fprintf(stderr, "Error: GetCommState()\n");
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
		fprintf(stderr, "Error: SetCommState()\n");

	ctTimeOuts.ReadIntervalTimeout = 250;
	ctTimeOuts.ReadTotalTimeoutMultiplier = 1; /* no good with big buffer */
	ctTimeOuts.ReadTotalTimeoutConstant = 500;
	ctTimeOuts.WriteTotalTimeoutMultiplier = 1;
	ctTimeOuts.WriteTotalTimeoutConstant = 5000;
	if(!SetCommTimeouts(ttyfd, &ctTimeOuts))
		fprintf(stderr, "Error: SetCommTimeouts()\n");
       
	Sleep(500);

	/* flush all pending input */
	if(!PurgeComm(ttyfd, PURGE_RXABORT | PURGE_RXCLEAR))
		fprintf(stderr, "Error: PurgeComm()\n");

#else /* _WIN32 */
	int ttyfd;
	struct termios oldtio, newtio;

        return_val_if_fail (ttyname != NULL, NULL);

	DEBUG(3, "%s() \n", __func__);

	info = calloc(1, sizeof(struct mobile_info));
	info->ttyname = ttyname;

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		fprintf(stderr, "Error: Can't open tty\n");
		free(info);
		return NULL;
	}

	/* probe mobile */
	tcgetattr(ttyfd, &oldtio);

	for (speed = 0; speeds[speed] != B0; speed++) {

		memset(&newtio, 0, sizeof(newtio));
		newtio.c_cflag = speeds[speed] | CS8 | CREAD;
		newtio.c_iflag = IGNPAR;
		newtio.c_oflag = 0;
		tcflush(ttyfd, TCIFLUSH);
		tcsetattr(ttyfd, TCSANOW, &newtio);

		/* is this ok? do we need to send both cr and lf? */
		if(do_at_cmd(ttyfd, "AT\r", rspbuf, sizeof(rspbuf)) < 0) {
			DEBUG(1, "Comm-error doing AT\n");
		}
		else if(strcasecmp("OK", rspbuf) == 0) {
			DEBUG(2, "OKAY doing AT (%s)\n", rspbuf);
			break;
		}

	}
	if(speeds[speed] == B0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Nothing found\n");
		return NULL;
	}
	info->speed = speeds[speed];

	/* detect mobile */
	if(do_at_cmd(ttyfd, "AT+GMI\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Comm-error doing AT+GMI\n");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->gmi = strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+GMM\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Comm-error doing AT+GMM\n");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->gmm = strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+GMR\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Comm-error doing AT+GMR\n");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->gmr= strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+CGMI\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Comm-error doing AT+CGMI\n");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->cgmi = strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+CGMM\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Comm-error doing AT+CGMM\n");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->cgmm = strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+CGMR\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		DEBUG(1, "Comm-error doing AT+CGMR\n");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->cgmr= strdup(rspbuf);

	close(ttyfd);
#endif
	return info;

}


int main(int argc, char *argv[])
{
	struct mobile_info *info;

	if  (mobile_param != NULL) { };

	if (argc > 1)
		info = probe_tty(argv[1]);
	else
#ifdef _WIN32
		info = probe_tty("COM1");
#else
		info = probe_tty("/dev/ttyS0");
#endif

	if (info != NULL) {
		printf("%s (%d):\n%s, %s, %s\n%s, %s, %s\n",
		       info->ttyname, info->speed,
		       info->gmi, info->gmm, info->gmr,
		       info->cgmi, info->cgmm, info->cgmr);
		
		return (EXIT_SUCCESS);
	}

	return (EXIT_FAILURE);
}
