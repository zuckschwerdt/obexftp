/*
 * cobex_bfb.h
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

#ifndef COBEXBFB_H
#define COBEXBFB_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	COBEX_BFB_LOG_DOMAIN	"cobex-bfb"

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

#endif /* COBEXBFB_H */
