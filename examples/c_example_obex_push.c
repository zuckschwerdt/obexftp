/*
	OBEX PUSH ObexFTP C client example
	Copyright (c) 2007 Christian W. Zuckschwerdt <zany@triq.net>
	Compile with:
	gcc -Wall $(pkg-config --libs obexftp) -o c_example_obex_push c_example_obex_push.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <obexftp/client.h> /*!!!*/

int main(int argc, char *argv[])
{
	char *device = NULL;
	int channel = -1;
	char *filepath, *filename;
	obexftp_client_t *cli = NULL; /*!!!*/
	int ret;

	/* Get the filename, device and optional channel */
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <filename> <bt_addr> [<channel>]\n", argv[0]);
		exit(1);
	}
	filepath = argv[1];
	device = argv[2];
	if (argc > 3)
		channel = atoi(argv[3]);
	else 
		channel = obexftp_browse_bt_push(device); /*!!!*/


	/* Extract basename from file path */
	filename = strrchr(filepath, '/');
	if (!filename)
		filename = filepath;
	else
		filename++;

	/* Open connection */
	cli = obexftp_open(OBEX_TRANS_BLUETOOTH, NULL, NULL, NULL); /*!!!*/
	if (cli == NULL) {
		fprintf(stderr, "Error opening obexftp client\n");
		exit(1);
	}

	/* Connect to device */
	ret = obexftp_connect_push(cli, device, channel); /*!!!*/
	if (ret < 0) {
		fprintf(stderr, "Error connecting to obexftp device\n");
		obexftp_close(cli);
		cli = NULL;
		exit(1);
	}

	/* Push file */
	ret = obexftp_put_file(cli, filepath, filename); /*!!!*/
	if (ret < 0) {
		fprintf(stderr, "Error putting file\n");
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

