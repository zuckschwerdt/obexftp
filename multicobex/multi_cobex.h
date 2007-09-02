/**
	\file multicobex/multi_cobex.h
	Detect, initiate and run OBEX over custom serial port protocols
	(Siemens, Ericsson, New-Siemens, Motorola, Generic).
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002-2007 Christian W. Zuckschwerdt <zany@triq.net>

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

#ifndef MULTICOBEX_H
#define MULTICOBEX_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* session handling */

obex_ctrans_t *
	cobex_ctrans (const char *tty);
void	cobex_free (obex_ctrans_t * ctrans);

/* callbacks */

int	cobex_connect (obex_t *self, void *data);
int	cobex_disconnect (obex_t *self, void *data);
int	cobex_write (obex_t *self, void *data, uint8_t *buffer, int length);
int	cobex_handleinput (obex_t *self, void *data, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* MULTICOBEX_H */
