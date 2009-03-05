/**
	\file obexftp/bt_kit_test.c
	Bluetooth, SDP, HCI kit for Linux, FreeBSD, NetBSD and Win32.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2007 Christian W. Zuckschwerdt <zany@triq.net>

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

/* gcc -Wall -I. -I../includes -DOBEXFTP_DEBUG=3 -DHAVE_BLUETOOTH -o bt_kit_test bt_kit.c bt_kit_test.c */
/* on win32 add: -lws2_32 */

#include <stdio.h>
#include <stdlib.h>

#include <bt_kit.h>

int main(int argc, char *argv[])
{
	int ret, i;
	char **list, *device;
	char *hci = NULL;
	int svclass = BTKIT_FTP_SERVICE;

	if (argc > 1) {
		hci = argv[1];
	}
	if (argc > 2) {
		svclass = atoi(argv[2]);
	}

	fprintf(stderr, "Initializing stack\n"); fflush(stderr);
	ret = btkit_init(); /* !!! */
	if (ret != 0) {
		fprintf(stderr, "btkit_init failed (%d)\n", ret); fflush(stderr);
	}


	fprintf(stderr, "Discovering devices\n"); fflush(stderr);
	list = btkit_discover(hci); /* !!! */
	if (list == NULL) {
		fprintf(stderr, "btkit_discover failed\n"); fflush(stderr);
	} else {
		for (i=0 ; list[i]; i++) {
			fprintf(stderr, "device %d: \"%s\"\n", i, list[i]); fflush(stderr);

			device = btkit_getname(hci, list[i]); /* !!! */
			fprintf(stderr, "bt name %d: \"%s\"\n", i, device); fflush(stderr);

			ret = btkit_browse(hci, list[i], svclass); /* !!! */
			fprintf(stderr, "service channel: %i\n", ret); fflush(stderr);
		}
		free(list);
	}

	
	fprintf(stderr, "Disposing stack\n"); fflush(stderr);
	ret = btkit_exit(); /* !!! */
	if (ret != 0) {
		fprintf(stderr, "btkit_exit failed (%d)\n", ret); fflush(stderr);
	}

	return 0;
}
