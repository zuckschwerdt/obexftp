/*
 *  apps/obexftp.c: Transfer from/to Mobile Equipment via OBEX
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
/*
 * Created at:    Don, 17 Jan 2002
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <sys/types.h>
#include <dirent.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <obexftp/uuid.h>
#include <cobexbfb/cobex_bfb.h>

#ifdef _WIN32_FIXME
/* OpenOBEX won't define a handler on win32 */
void DEBUG(unsigned int n, ...) { }
void DUMPBUFFER(unsigned int n, char *label, char *msg) { }
#endif /* _WIN32 */

#include <common.h>

#ifdef HAVE_BLUETOOTH
#include "bt_discovery.h"
#endif

#define OBEXFTP_PORT "OBEXFTP_PORT"
#define OBEXFTP_ADDR "OBEXFTP_ADDR"

/* current command, set by main, read from info_cb */
int c;

static void info_cb(int event, const char *msg, /*@unused@*/ int len, /*@unused@*/ void *data)
{
	char progress[] = "\\|/-";
	static unsigned int i = 0;

	switch (event) {

	case OBEXFTP_EV_ERRMSG:
		fprintf(stderr, "Error: %s\n", msg);
		break;

	case OBEXFTP_EV_ERR:
		fprintf(stderr, "failed: %s\n", msg);
		break;
	case OBEXFTP_EV_OK:
		fprintf(stderr, "done\n");
		break;

	case OBEXFTP_EV_CONNECTING:
		fprintf(stderr, "Connecting...");
		break;
	case OBEXFTP_EV_DISCONNECTING:
		fprintf(stderr, "Disconnecting...");
		break;
	case OBEXFTP_EV_SENDING:
		fprintf(stderr, "Sending %s... ", msg);
		break;
	case OBEXFTP_EV_RECEIVING:
		fprintf(stderr, "Receiving %s... ", msg);
		break;

	case OBEXFTP_EV_LISTENING:
		fprintf(stderr, "Waiting for incoming connection\n");
		break;

	case OBEXFTP_EV_CONNECTIND:
		fprintf(stderr, "Incoming connection\n");
		break;
	case OBEXFTP_EV_DISCONNECTIND:
		fprintf(stderr, "Disconnecting\n");
		break;

	case OBEXFTP_EV_INFO:
		printf("Got info %d: \n", (int)msg);
		break;

	case OBEXFTP_EV_BODY:
		if (c == 'l')
			write(STDOUT_FILENO, msg, len);
		break;

	case OBEXFTP_EV_PROGRESS:
		fprintf(stderr, "%c%c", 0x08, progress[i++]);
		fflush(stdout);
		if (i >= strlen(progress))
			i = 0;
		break;

	}
}

/*@only@*/ /*@null@*/ static obexftp_client_t *cli = NULL;
static int transport = OBEX_TRANS_IRDA;
/*@only@*/ /*@null@*/ static char *tty = NULL;
/*@only@*/ /*@null@*/ static char *btaddr = NULL;
static int btchannel = 10;

static int cli_connect()
{
/*@only@*/ /*@null@*/ static obex_ctrans_t *ctrans = NULL;
	int retry;

	if (cli != NULL)
		return TRUE;

	if (tty != NULL) {
       		ctrans = cobex_ctrans (tty);
       		fprintf(stderr, "Custom transport set to 'Siemens/Ericsson'\n");
	}
	else {
		ctrans = NULL;
		fprintf(stderr, "No custom transport\n");
	}

	/* Open */
	cli = obexftp_cli_open (transport, ctrans, info_cb, NULL);
	if(cli == NULL) {
		fprintf(stderr, "Error opening obexftp-client\n");
		exit(1);
		//return FALSE;
	}

	for (retry = 0; retry < 3; retry++) {

		/* Connect */
		if (obexftp_cli_connect (cli, btaddr, btchannel) >= 0)
			return TRUE;
		fprintf(stderr, "Still trying to connect\n");
	}

	cli = NULL;
	exit(1);
	//return FALSE;
}

static void cli_disconnect()
{
	if (cli != NULL) {
		/* Disconnect */
		(void) obexftp_cli_disconnect (cli);
		/* Close */
		obexftp_cli_close (cli);
	}
}

int main(int argc, char *argv[])
{
	int verbose=0;
	int most_recent_cmd = 0;
	char *target_path = NULL;
	char *move_src = NULL;
	/* char *inbox; */

	/* preset mode of operation depending on our name */
	if (strstr(argv[0], "ls") != NULL)	most_recent_cmd = 'l';
	if (strstr(argv[0], "get") != NULL)	most_recent_cmd = 'g';
	if (strstr(argv[0], "put") != NULL)	most_recent_cmd = 'p';
	if (strstr(argv[0], "mv") != NULL)	most_recent_cmd = 'm';
	if (strstr(argv[0], "rm") != NULL)	most_recent_cmd = 'k';

	/* preset the port for environment */
	tty = getenv(OBEXFTP_PORT);
	if (tty != NULL)
		tty = strdup(tty);
	btaddr = getenv(OBEXFTP_ADDR);
	if (btaddr != NULL)
		btaddr = strdup(btaddr);
	       

	/* by default don't debug anything */
	/*
	log_handler = g_log_set_handler (NULL,
					 G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO,
					 g_log_null_handler, NULL);
	*/
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"irda",	no_argument, NULL, 'i'},
#ifdef HAVE_BLUETOOTH
			{"bluetooth",	optional_argument, NULL, 'b'},
#endif
			{"channel",	required_argument, NULL, 'B'},
			{"tty",		required_argument, NULL, 't'},
			{"list",	optional_argument, NULL, 'l'},
			{"chdir",	required_argument, NULL, 'c'},
			{"mkdir",	required_argument, NULL, 'C'},
			{"path",	required_argument, NULL, 'f'},
			{"get",		required_argument, NULL, 'g'},
			{"getdelete",	required_argument, NULL, 'G'},
			{"put",		required_argument, NULL, 'p'},
			{"delete",	required_argument, NULL, 'k'},
			{"info",	no_argument, NULL, 'x'},
			{"move",	required_argument, NULL, 'm'},
			{"verbose",	no_argument, NULL, 'v'},
			{"version",	no_argument, NULL, 'V'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'u'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-ib::B:t:L::l::c:C:f:g:G:p:k:xm:Vvh",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;
	
		switch (c) {
		
		case 'i':
			transport = OBEX_TRANS_IRDA;
			if (tty != NULL)
				free (tty);
			break;
		
#ifdef HAVE_BLUETOOTH
		case 'b':
			transport = OBEX_TRANS_BLUETOOTH;
			if (tty != NULL)
				free (tty);
			if (btaddr != NULL)
				free (btaddr);
       			//btaddr = optarg;
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
			}
			discover_bt(optarg, &btaddr, &btchannel);
			//fprintf(stderr, "Got %s channel %d\n", btaddr, btchannel);
			break;
			
		case 'B':
			btchannel = atoi(optarg);
			break;
#endif /* HAVE_BLUETOOTH */
		
		case 't':
			transport = OBEX_TRANS_CUSTOM;
			if (tty != NULL)
				free (tty); /* ok to to free an optarg? */
       			tty = optarg; /* strdup? */

			if (strstr(optarg, "ir") != NULL)
				fprintf(stderr, "Do you really want to use IrDA via ttys?\n");
			break;

		case 'L':
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
			}
			if(cli_connect ()) {
				/* List folder */
				struct dirent *ent;
				DIR *dir = obexftp_opendir(cli, optarg);
				while ((ent = obexftp_readdir(dir)) != NULL) {
					struct stat st;
					obexftp_stat(cli, ent->d_name, &st);
					printf("%ld %s (%d)\n", st.st_size, ent->d_name, ent->d_type);
				}
				obexftp_closedir(dir);
			}
			most_recent_cmd = c;
			break;

		case 'l':
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
			}
			if(cli_connect ()) {
				/* List folder */
				(void) obexftp_list(cli, NULL, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'c':
			if(cli_connect ()) {
				/* Change dir */
				(void) obexftp_setpath(cli, optarg, 0);
			}
			most_recent_cmd = c;
			break;

		case 'C':
			if(cli_connect ()) {
				/* Change or Make dir */
				(void) obexftp_setpath(cli, optarg, 1);
			}
			most_recent_cmd = c;
			break;

		case 'f':
			target_path = optarg;
			break;

		case 'g':
			if(cli_connect ()) {
				char *p;
				/* Get file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				(void) obexftp_get(cli, p, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'G':
			if(cli_connect ()) {
				char *p;
				/* Get file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				if (obexftp_get(cli, p, optarg))
					(void) obexftp_del(cli, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'p':
			if(cli_connect ()) {
				char *basename;
                		basename = strrchr(optarg, '/');
		                if (basename)
                		        basename++;
				else
					basename = optarg;
				/* Send file */
				(void) obexftp_put_file(cli, optarg, basename);
			}
			most_recent_cmd = c;
			break;

		case 'k':
			if(cli_connect ()) {
				/* Delete file */
				(void) obexftp_del(cli, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'x':
			if(cli_connect ()) {
				/* for S65 */
				(void) obexftp_cli_disconnect (cli);
				(void) obexftp_cli_connect_uuid (cli, btaddr, btchannel, UUID_S45);
				/* Retrieve Infos */
				(void) obexftp_info(cli, 0x01);
				(void) obexftp_info(cli, 0x02);
			}
			most_recent_cmd = 'h'; // not really
			break;

		case 'm':
			most_recent_cmd = c;

			if (move_src == NULL) {
				move_src = optarg;
				break;
			}
			if(cli_connect ()) {
				/* Rename a file */
				(void) obexftp_rename(cli, move_src, optarg);
			}
			move_src = NULL;
			break;

		case 'v':
			verbose++;
			break;
			
		case 'V':
			printf("ObexFTP 0.10.4\n");
			most_recent_cmd = 'h'; // not really
			break;

		case 'h':
		case 'u':
			printf("ObexFTP 0.10.4\n");
			printf("Usage: %s [-i | -b <dev> [-B <chan>] | -t <dev>] [-l <dir> ...] [-c <dir>]\n"
				"[-g <file> ...] [-p <files> ...] [-k <files> ...] [-x] [-m <src> <dest> ...]\n"
				"Transfer files from/to Mobile Equipment.\n"
				"Copyright (c) 2002-2004 Christian W. Zuckschwerdt\n"
				"\n"
				" -i, --irda                  connect using IrDA transport (default)\n"
#ifdef HAVE_BLUETOOTH
				" -b, --bluetooth [<device>]  use or search a bluetooth device\n"
				" [ -B, --channel <number> ]  use this bluetooth channel when connecting\n"
#endif
				" -t, --tty <device>          connect to this tty using a custom transport\n\n"
				" -l, --list [<FOLDER>]       list current/given folder\n"
				" -c, --chdir <DIR>           chdir\n"
				" -C, --mkdir <DIR>           mkdir and chdir\n"
				// " -f, --path <PATH>           specify the local file name or directory\n"
				"                             get and put always specify the remote name.\n"
				" -g, --get <SOURCE>          fetch files\n"
				" -G, --getdelete <SOURCE>    fetch and delete (move) files \n"
				" -p, --put <SOURCE>          send files\n"
				" -k, --delete <SOURCE>       delete files\n\n"
				" -x, --info                  retrieve infos (Siemens)\n"
				" -m, --move <SRC> <DEST>     move files (Siemens)\n\n"
				" -v, --verbose               verbose messages\n"
				" -V, --version               print version info\n"
				" -h, --help, --usage         this help text\n"
				"\n",
				argv[0]);
			most_recent_cmd = 'h'; // not really
			break;

		default:
			printf("Try `%s --help' for more information.\n",
				 argv[0]);
		}
	}

	if (most_recent_cmd == 0)
	       	fprintf(stderr, "Nothing to do. Use --help for help.\n");

	if (optind < argc) {
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
	}

	cli_disconnect ();

	exit (0);

}
