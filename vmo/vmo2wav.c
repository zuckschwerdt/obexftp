/*
 *  apps/vmo2wav.c: Decode VMO files to wav format
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
 *
 * very bad hack! Introduce dependancies on libgsm and wav lib later.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h> /* ntohs */

#include "gsm.h"

#define FRAME_SIZE 34

uint8_t format[] = {0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,  0x40, 0x1f, 0x00, 0x00, 0x80, 0x3e, 0x00, 0x00,  0x02, 0x00, 0x10, 0x00};

int main(int argc, char *argv[])
{
	int i;
	uint8_t frame[FRAME_SIZE];
	int l;
	int bytes;
	int in;
	struct stat st;
	int size;

	gsm handle;
	gsm_frame buf;
	gsm_signal sample[160];

	for (i = 1; i < argc; i++) {

		if (!(handle = gsm_create())) { /* error... */ }

		in = open (argv[i], O_RDONLY);
		fstat (in, &st);

		write(STDOUT_FILENO, "RIFF", 4);
		size = st.st_size/34*160*2 + 36;
		write(STDOUT_FILENO, &size, 4);
		write(STDOUT_FILENO, "WAVEfmt ", 8);
		write(STDOUT_FILENO, format, 20);
		write(STDOUT_FILENO, "data", 4);
		size = st.st_size/34*160*2;
		write(STDOUT_FILENO, &size, 4);

		do {
			bytes = read (in, &frame, FRAME_SIZE);
			if (bytes == FRAME_SIZE) {

			buf[0] = (frame[0] & 0x0f) | 0xd0;
			for (l = 0; l < 16; l++) {
				/* frame[l*2] = ntohs (frame[l*2]); */
				buf[2*l+1] = frame[2*l+3];
				buf[2*l+2] = frame[2*l+2];
			}

			if (gsm_decode(handle, buf, sample) < 0) {
				/* error... */
			}
			if (write(STDOUT_FILENO, sample, sizeof sample) != sizeof sample) {
				/* error... */
			}
			}
		} while (bytes == FRAME_SIZE);
		gsm_destroy(handle);
		close (in);
	}

	exit (EXIT_SUCCESS);

}
