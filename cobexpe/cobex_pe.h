/*
 *                
 * Filename:      cobex_pe.h
 * Version:       
 * Description:   
 * Status:        Experimental.
 * Author:        Pontus Fuchs <pontus@tactel.se>
 * Created at:    Wed Nov 17 22:05:16 1999
 * Modified at:   Mon, 29 Apr 2002 22:58:53 +0100
 * Modified by:   Christian W. Zuckschwerdt <zany@triq.net>
 *
 *   Copyright (c) 1998, 1999, Dag Brattli, All Rights Reserved.
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

#ifndef COBEXPE_H
#define COBEXPE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	COBEX_PE_LOG_DOMAIN	"cobex-pe"

/* session handling */

obex_ctrans_t *
	cobex_pe_ctrans (const char *tty);
void	cobex_pe_free (/*@only@*/ /*@null@*/ obex_ctrans_t * ctrans);

/* callbacks */

int	cobex_pe_connect (obex_t *self, void *data);
int	cobex_pe_disconnect (obex_t *self, void *data);
int	cobex_pe_write (obex_t *self, void *data, uint8_t *buffer, int length);
int	cobex_pe_handleinput (obex_t *self, void *data, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* COBEXPE_H */
