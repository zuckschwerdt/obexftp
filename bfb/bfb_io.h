/**
	\file bfb/bfb_io.h
	BFB protocol IO layer.
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

#ifndef BFB_IO_H
#define BFB_IO_H

#include "bfb/bfb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Send an BFB init command an check for a valid answer frame */
int bfb_io_init(fd_t fd);

int bfb_io_read(fd_t fd, void *buffer, int length, int timeout);
int bfb_io_write(fd_t fd, const void *buffer, int length, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* BFB_IO_H */
