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

	if (argc > 1) {
		hci = argv[1];
	}

	ret = btkit_init();
	if (ret != 0) {
		fprintf(stderr, "btkit_init failed (%d)\n", ret);
	}


	list = btkit_discover(hci);
	if (list == NULL) {
		fprintf(stderr, "btkit_discover failed\n");
	} else {
		for (i=0 ; list[i]; i++) {
			fprintf(stderr, "device %d: \"%s\"\n", i, list[i]);

			device = btkit_getname(hci, list[i]);
			fprintf(stderr, "bt name %d: \"%s\"\n", i, device);

//			ret = btkit_browse(hci, device, 0x1106 /*OBEX FTP*/);
		}
		free(list);
	}

	
	ret = btkit_exit();
	if (ret != 0) {
		fprintf(stderr, "btkit_exit failed (%d)\n", ret);
	}

	return 0;
}
