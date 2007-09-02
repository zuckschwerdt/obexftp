/**
	\file multicobex/multi_cobex_private.h
	Detect, initiate and run OBEX over custom serial port protocols
	(Siemens, Ericsson, New-Siemens, Motorola, Generic).
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002-2005 Christian W. Zuckschwerdt <zany@triq.net>

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

#ifndef MULTICOBEX_PRIVATE_H
#define MULTICOBEX_PRIVATE_H

#include <bfb/bfb.h>

#define SERPORT "/dev/ttyS0"

#define	RECVSIZE 500		/* Recieve up to this much from socket */

enum cobex_type
{
        CT_BFB,			/* use a bfb transport */
        CT_ERICSSON,		/* just custom init and teardown */
        CT_SIEMENS,		/* new siemens, like ericsson above */
        CT_MOTOROLA,		/* experimental motorola support */
        CT_GENERIC		/* should work on most phones */
};

typedef struct {
	enum cobex_type type;	/* Type of connected mobile */
	char *tty;
#ifdef _WIN32
	HANDLE fd;		/* Socket descriptor */
#else
	int fd;			/* Socket descriptor */
#endif
	uint8_t recv[RECVSIZE];	/* Buffer socket input */
	int recv_len;
	uint8_t seq;
	bfb_data_t *data_buf;	/* assembled obex frames */
	int data_size;		/* max buffer size */
	int data_len;		/* filled buffer length */
} cobex_t;

#endif /* MULTICOBEX_PRIVATE_H */
