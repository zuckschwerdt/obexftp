/*
 *  bfb/bfb_io.h
 *
 *  Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
 * 
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */

#include <glib.h>

/* Read a BFB answer */
gint	do_bfb_read(int fd, guint8 *buffer, gint length);

/* Send an BFB init command an check for a valid answer frame */
gboolean
	do_bfb_init(int fd);

/* Send an AT-command an expect 1 line back as answer */
gint	do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen);

/* close the connection */
void	bfb_io_close(int fd, int force);

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
gint	bfb_io_open(const gchar *ttyname);

