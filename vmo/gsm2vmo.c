/*
 *  apps/gsm2vmo.c: Convert GSM files to VMO format
 *
 *  Copyright (c) 2003 Christian W. Zuckschwerdt <zany@triq.net>
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
/*
 * Created at: 2003-02-17
 *
 * vmo format courtesty of Dmitry Zakharov <dmit@crp.bank.gov.ua>
 *
 * The file consists of 34-byte frames in network byte order.
 * 12 bit header and 260 bits standart GSM frame, as described
 * in GSM 06.10 and GSM 06.12. The header is 0x2010 always.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h> /* ntohs */

#define FRAME_SIZE 34

int main(int argc, char *argv[])
{
	int i;
	char fn[PATH_MAX+1];
	int in, out;
	uint8_t frame[FRAME_SIZE];
	int l;
	int bytes;

	for (i = 1; i < argc; i++) {
		printf("Converting %s ", argv[i]);
		snprintf(fn, PATH_MAX, "%s.vmo", argv[i]);
		in = open (argv[i], O_RDONLY);
		out = creat (fn, 0644);
		do {
			uint8_t swap;
			bytes = read (in, &frame[1], FRAME_SIZE-1);
			frame[0] = 0x20;
			frame[1] &= 0x0f;
			frame[1] |= 0x10;
			for (l = 0; l < 17; l++) {
				/* frame[l*2] = ntohs (frame[l*2]); */
				swap = frame[2*l];
				frame[2*l] = frame[2*l+1];
				frame[2*l+1] = swap;
			}
			/*
			printf ("frame: %02x %02x %02x%02x (%d)\n",
				frame[0], frame[1],
				frame[32], frame[33], bytes);
			*/
			write (out, &frame, bytes+1);
			printf(".");
		} while (bytes == FRAME_SIZE-1);
		close (in);
		close (out);
		printf("\n");
	}

	exit (EXIT_SUCCESS);

}
