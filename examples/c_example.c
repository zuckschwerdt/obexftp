/*
	Minimal ObexFTP C client example
	Copyright (c) 2007 Christian W. Zuckschwerdt <zany@triq.net>
	Compile with:
	gcc -Wall $(pkg-config --libs obexftp) -o c_example c_example.c
 */

#include <stdio.h>
#include <stdlib.h>

#include <obexftp/client.h> /*!!!*/

int main(int argc, char *argv[])
{
	char *device;
	int channel;
	char *filename = NULL;
	obexftp_client_t *cli = NULL; /*!!!*/ 
	int ret;

	/* Get device, channel and optional filename */
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <bt_addr> <channel> [<filename>]\n", argv[0]);
		exit(1);
	}
	device = argv[1];
       	channel = atoi(argv[2]);
	if (argc > 3) {
		filename = argv[3];
	}

	/* Open connection */
	cli = obexftp_open(OBEX_TRANS_BLUETOOTH, NULL, NULL, NULL); /*!!!*/ 
	if (cli == NULL) {
		fprintf(stderr, "Error opening obexftp client\n");
		exit(1);
	}

	/* Connect to device */
	ret = obexftp_connect(cli, device, channel); /*!!!*/ 
	if (ret < 0) {
		fprintf(stderr, "Error connecting to obexftp device\n");
		obexftp_close(cli);
		cli = NULL;
		exit(1);
	}

	if (filename == NULL) {
		/* List folder */
		ret = obexftp_list(cli, NULL, "/"); /*!!!*/ 
		if (ret < 0) {
			fprintf(stderr, "Error getting a folder listing\n");
		} else {
			printf("%s\n", cli->buf_data); /*!!!*/ 
		}

	} else {
		/* Get file */
		ret = obexftp_get(cli, NULL, filename); /*!!!*/ 
		if (ret < 0) {
			fprintf(stderr, "Error getting a file\n");
		} else {
			/* do something with cli->buf_data and cli->buf_size */
		}
	}

	/* Disconnect */
	ret = obexftp_disconnect(cli); /*!!!*/ 
       	if (ret < 0) {
       		fprintf(stderr, "Error disconnecting the client\n");
       	}

	/* Close */
	obexftp_close(cli); /*!!!*/
	cli = NULL;

	exit(0);
}
