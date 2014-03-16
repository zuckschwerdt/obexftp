/**
	\file multicobex/multi_cobex_io.h
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

#ifndef MULTICOBEX_IO_H
#define MULTICOBEX_IO_H

#include <inttypes.h>
#include "multi_cobex_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef fd_t_defined
  #ifdef _WIN32
    #include <windows.h>
    typedef HANDLE fd_t;
    #define fd_t_defined
  #else
    typedef int fd_t;
    #define fd_t_defined
  #endif
#endif

#if !defined(_WIN32) && !defined(INVALID_HANDLE_VALUE)
#define INVALID_HANDLE_VALUE -1
#endif

/* Write out a buffer */
int cobex_io_write(fd_t fd, const void *buffer, int length);

/* Read in a answer */
int cobex_io_read(fd_t fd, void *buffer, int length, int timeout);

/* Send an BFB init command an check for a valid answer frame */
int cobex_io_init(fd_t fd);

/* close the connection */
void cobex_io_close(fd_t fd, enum cobex_type typeinfo, int force);

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
fd_t cobex_io_open(const char *ttyname, enum cobex_type *typeinfo);

#ifdef __cplusplus
}
#endif

#endif /* MULTICOBEX_IO_H */
