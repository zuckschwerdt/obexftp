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

#include <inttypes.h>

enum trans_type
{
        TT_BFB,			/* use a bfb transport */
        TT_ERICSSON,		/* just custom init and teardown */
        TT_SIEMENS,		/* new siemens, like ericsson above */
        TT_MOTOROLA,		/* experimental motorola support */
        TT_GENERIC		/* should work on most phones */
};

#ifdef __cplusplus
extern "C" {
#endif

/* Write out a BFB buffer */
int	bfb_io_write(fd_t fd, const uint8_t *buffer, int length);

/* Read in a BFB answer */
int	bfb_io_read(fd_t fd, uint8_t *buffer, int length, int timeout);

/* Send an BFB init command an check for a valid answer frame */
int	bfb_io_init(fd_t fd);

/* Send an AT-command an expect 1 line back as answer */
int	do_at_cmd(fd_t fd, const char *cmd, char *rspbuf, int rspbuflen);

/* close the connection */
void	bfb_io_close(fd_t fd, int force);

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
fd_t	bfb_io_open(const char *ttyname, enum trans_type *typeinfo);

#ifdef __cplusplus
}
#endif

#endif /* BFB_IO_H */
