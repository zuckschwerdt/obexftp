/*
 *  apps/obexftp_cli.c: Transfer from/to Siemens Mobile Equipment via OBEX
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
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <cobexbfb/cobex_bfb.h>
/* waring: cobexpe serial io is broken on win32 */
#include <cobexpe/cobex_pe.h>

#ifdef _WIN32_FIXME
/* OpenOBEX won't define a handler on win32 */
void DEBUG(unsigned int n, ...) { }
void DUMPBUFFER(unsigned int n, char *label, char *msg) { }
#endif /* _WIN32 */

#include <common.h>

#define OBEXFTP_PORT "OBEXFTP_PORT"
#define OBEXFTP_ADDR "OBEXFTP_ADDR"

#if 0
void g_log_null_handler (const char *log_domain,
			 GLogLevelFlags log_level,
			 const char *message,
			 void *user_data) {
}

void g_log_print_handler (const char *log_domain,
			 GLogLevelFlags log_level,
			 const char *message,
			 void *user_data) {
	g_print ("%s\n", message);
}
#endif

/* current command, set my main, read from info_cb */
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
		fprintf(stderr, "Got info %d: \n", (int)msg);
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
		return FALSE;
	}

	for (retry = 0; retry < 3; retry++) {

		/* Connect */
		if (obexftp_cli_connect (cli, btaddr, btchannel) >= 0)
			return TRUE;
		fprintf(stderr, "Still trying to connect\n");
	}

	cli = NULL;
	return FALSE;
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
	/* int verbose=0; */
	/* guint log_handler; */
	int most_recent_cmd = 0;
	char *p;
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
			{"bluetooth",	required_argument, NULL, 'b'},
			{"tty",		required_argument, NULL, 't'},
			{"list",	optional_argument, NULL, 'l'},
			{"chdir",	required_argument, NULL, 'c'},
			{"get",		required_argument, NULL, 'g'},
			{"put",		required_argument, NULL, 'p'},
			{"delete",	required_argument, NULL, 'k'},
			{"info",	no_argument, NULL, 'x'},
			{"move",	required_argument, NULL, 'm'},
			{"verbose",	no_argument, NULL, 'v'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'u'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-ib:t:l::c:g:p:k:xm:vh",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;

		switch (c) {
		
		case 'i':
			transport = OBEX_TRANS_IRDA;
			break;
		
		case 'b':
			transport = OBEX_TRANS_BLUETOOTH;
			if (btaddr != NULL)
				free (btaddr);
       			btaddr = optarg;
			break;
		
		case 't':
			transport = OBEX_TRANS_CUSTOM;
			if (tty != NULL)
				free (tty);

			if (!strcasecmp(optarg, "irda"))
				tty = NULL;
			else
				tty = optarg;
			break;

		case 'l':
			if(cli_connect ()) {
				/* List folder */
				(void) obexftp_list(cli, NULL, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'c':
			if(cli_connect ()) {
				/* Get file */
				(void) obexftp_setpath(cli, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'g':
			if(cli_connect ()) {
				/* Get file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				(void) obexftp_get(cli, p, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'p':
			if(cli_connect ()) {
				/* Send file */
				(void) obexftp_put_file(cli, optarg, optarg);
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
				/* Retrieve Infos */
				(void) obexftp_info(cli, 0x01);
				(void) obexftp_info(cli, 0x02);
			}
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
#if 0
			if (verbose++ > 0)
				log_handler = g_log_set_handler (NULL,
					 G_LOG_LEVEL_DEBUG,
					 g_log_default_handler, NULL);
			else
				/* remove muting handler? */
				/* g_log_remove_handler (NULL, log_handler); */
				log_handler = g_log_set_handler (NULL,
					 G_LOG_LEVEL_INFO,
					 g_log_default_handler, NULL);
#endif
			break;
		case 'h':
		case 'u':
			printf("Usage: %s [-d <dev>] [-s|-a] [-l <dir> ...] [-c <dir>]\n"
				"[-g <file> ...] [-p <files> ...] [-i] [-m <src> <dest> ...] [-k <files> ...]\n"
				"Transfer files from/to Siemens Mobile Equipment.\n"
				"Copyright (c) 2002-2003 Christian W. Zuckschwerdt\n"
				"\n"
				" -i, --irda                  connect using IrDA transport\n"
				" -b, --bluetooth <device>    connect to this bluetooth device\n"
				" -t, --tty <device>          connect to this tty using a custom transport\n"
				" -l, --list [<FOLDER>]       list a folder\n"
				" -c, --chdir <DIR>           chdir / mkdir\n"
				" -g, --get <SOURCE>          fetch files\n"
				" -p, --put <SOURCE>          send files\n"
				" -k, --delete <SOURCE>       delete files\n"
				" -x, --info                  retrieve infos (Siemens)\n\n"
				" -m, --move <SRC> <DEST>     move files (Siemens)\n"
				" -v, --verbose               verbose messages\n"
				" -h, --help, --usage         this help text\n"
				"\n",
				argv[0]);
			break;

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
	}

	cli_disconnect ();

	exit (0);

}
