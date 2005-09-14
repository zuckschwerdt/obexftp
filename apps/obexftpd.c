/*
 *  apps/obexftpd.c: Transfer from/to Mobile Equipment via OBEX
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
 * Created at:    Don, 2 Okt 2003
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>

/* just until there is a server layer in obexftp */
#include <openobex/obex.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <cobexbfb/cobex_bfb.h>
#include <common.h>

/* current command, set by main, read from info_cb */
int c;

volatile int finished = 0;
volatile int success = 0;

static void obex_event(obex_t *handle, obex_object_t *obj, int mode, int event, int obex_cmd, int obex_rsp)
{
	char progress[] = "\\|/-";
	static unsigned int i = 0;

	switch (event) {
	case OBEX_EV_STREAMAVAIL:
                printf("Time to read some data from stream\n");
                /* ret = ircp_srv_receive(srv, object, FALSE); */
                break;

	case OBEX_EV_LINKERR:
                finished = 1;
                success = FALSE;
		fprintf(stderr, "failed: %d\n", obex_cmd);
		break;

        case OBEX_EV_REQ:
                printf("Incoming request %02x, %d\n", obex_cmd, OBEX_CMD_SETPATH);

	case OBEX_EV_REQHINT:
                /* An incoming request is about to come. Accept it! */

	case OBEX_EV_REQDONE:
                finished = TRUE;
                if(obex_rsp == OBEX_RSP_SUCCESS)
                        success = TRUE;
                else {
                        success = FALSE;
                        printf("%s() OBEX_EV_REQDONE: obex_rsp=%02x\n", __func__, obex_rsp);
                }
                /* client_done(handle, object, obex_cmd, obex_rsp); */
		break;

	case OBEX_EV_PROGRESS:
		fprintf(stderr, "%c%c", 0x08, progress[i++]);
		fflush(stdout);
		if (i >= strlen(progress))
			i = 0;
		break;

        default:
                printf("%s() Unhandled event %d\n", __func__, event);
                break;

	}
}

/*@only@*/ /*@null@*/ static char *tty = NULL;

bdaddr_t *bt_src =NULL;
uint8_t bt_channel = 10; /* OBEX_PUSH_HANDLE */

static void start_server(int transport)
{
	obex_t *handle = NULL;
/*@only@*/ /*@null@*/ static obex_ctrans_t *ctrans = NULL;

	if (tty != NULL) {
       		ctrans = cobex_ctrans (tty);
       		fprintf(stderr, "Custom transport set to 'Siemens/Ericsson'\n");
	}
	else {
		ctrans = NULL;
		fprintf(stderr, "No custom transport\n");
	}

	handle = OBEX_Init(transport, obex_event, 0);


	BtOBEX_ServerRegister(handle, bt_src, bt_channel);
	printf("Waiting for connection...\n");

	// this causes trouble: (void) OBEX_ServerAccept(handle, obex_event, NULL);

	while (!finished) {
		printf("Handling connection...\n");
		OBEX_HandleInput(handle, 1);
	}

	OBEX_Cleanup(handle);
}

int main(int argc, char *argv[])
{
	int verbose=0;
	int most_recent_cmd = 0;
	/* char *inbox; */

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"irda",	no_argument, NULL, 'i'},
			{"bluetooth",	no_argument, NULL, 'b'},
			{"tty",		required_argument, NULL, 't'},
			{"network",	required_argument, NULL, 'N'},
			{"chdir",	required_argument, NULL, 'c'},
			{"verbose",	no_argument, NULL, 'v'},
			{"version",	no_argument, NULL, 'V'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'u'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-ibt:c:Vvh",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;

		switch (c) {
		
		case 'i':
			start_server(OBEX_TRANS_IRDA);
			break;
		
		case 'b':
			start_server(OBEX_TRANS_BLUETOOTH);
			break;
		
		case 't':
			printf("accepting on tty not implemented yet.");
			/* start_server(OBEX_TRANS_CUSTOM); optarg */
			break;

		case 'N':
			//transport = OBEX_TRANS_INET;
			//if (inethost != NULL)
			//	free (inethost); /* ok to to free an optarg? */
       			//inethost = optarg; /* strdup? */

			{
				int n;
				if (sscanf(optarg, "%d.%d.%d.%d", &n, &n, &n, &n) != 4)
					fprintf(stderr, "Please use dotted quad notation.\n");
			}
			start_server(OBEX_TRANS_INET);
			break;

		case 'c':
			chdir(optarg);
			break;

		case 'v':
			verbose++;
			break;

		case 'V':
			printf("ObexFTPd %s\n", VERSION);
			most_recent_cmd = 'h'; // not really
			break;

		case 'h':
		case 'u':
			printf("ObexFTPd %s\n", VERSION);
			printf("Usage: %s [-v] [-c <path>] [-i] [-b] [-t <dev>]\n"
				"Recieve files from/to Mobile Equipment.\n"
				"Copyright (c) 2003 Christian W. Zuckschwerdt\n"
				"\n"
				" -v, --verbose               verbose messages\n"
				" -c, --chdir <DIR>           set a default basedir\n"
				" -i, --irda                  accept IrDA connections\n"
				" -b, --bluetooth             accept bluetooth connections\n"
				" -t, --tty <device>          accept connections from this tty\n"
				" -V, --version               print version info\n"
				" -h, --help, --usage         this help text\n"
				"\n"
				"THIS PROGRAMM IS NOT WORKING YET\n"
				"\n",
				argv[0]);
			exit(0);

		default:
			printf("Try `%s --help' for more information.\n",
				 argv[0]);
		}
	}

	if (optind < argc) {
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
	exit (0);
	}

	exit (0);

}
