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
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#define INVALID_HANDLE_VALUE -1
#endif

#include "bfb.h"
#include "bfb_io.h"
#include <common.h>

int bfb_io_write(fd_t fd, const void *buffer, int length, int timeout)
{
	struct timeval time;
	fd_set fds;
	int rc;

        if (fd == INVALID_HANDLE_VALUE) {
		DEBUG(1, "%s() Error file handle invalid\n", __func__);
		return -1;
	}

	/* select setup */
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	/* Set time limit. */
	time.tv_sec = timeout;
	time.tv_usec = 0;

	rc = select(fd+1, NULL, &fds, NULL, &time);
	if ( rc > 0) {
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

int bfb_io_read(fd_t fd, void *buffer, int length, int timeout)
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
		if (actual < 0) {
			DEBUG(2, "%s() Read error: %d\n", __func__, actual);
		}
		return actual;
	}
	else {
		DEBUG(1, "%s() No data (timeout: %d)\n", __func__, timeout);
		return 0;
	}
#endif
}

int bfb_io_init(fd_t fd)
{
	int actual;
	int tries=3;
	bfb_frame_t *frame = NULL;
	uint8_t rspbuf[200];
	int rsplen;

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

	while (!frame && tries-- > 0) {
		actual = bfb_write_packets (fd, BFB_FRAME_CONNECT, &init_magic, sizeof(init_magic));
		DEBUG(2, "%s() Wrote %d packets\n", __func__, actual);

		if (actual < 1) {
			DEBUG(1, "BFB port error\n");
			return FALSE;
		}

		rsplen = 0;
		while (!frame && actual > 0) {
			actual = bfb_io_read(fd, &rspbuf[rsplen], sizeof(rspbuf)-rsplen, 2);
			DEBUG(2, "%s() Read %d bytes\n", __func__, actual);

			if (actual < 0) {
				DEBUG(1, "BFB read error\n");
				return FALSE;
			}
			if (actual == 0) {
				DEBUG(1, "BFB read timeout\n");
			}

			rsplen += actual;
			frame = bfb_read_packets(rspbuf, &rsplen);
			DEBUG(2, "%s() Unstuffed, %d bytes remaining\n", __func__, rsplen);
		}
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
