/*
 *                
 * Filename:      cobex_bfb_private.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 * Modified at:   Don, 17 Jan 2002 23:46:52 +0100
 * Modified by:   Christian W. Zuckschwerdt <zany@triq.net>
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

#ifndef COBEXBFB_PRIVATE_H
#define COBEXBFB_PRIVATE_H

#include <bfb/bfb.h>

#define SERPORT "/dev/ttyS0"

#define	RECVSIZE 500	/* Recieve up to this much from socket */

typedef struct {
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

#endif /* COBEXBFB_PRIVATE_H */
