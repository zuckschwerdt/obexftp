/*
 *                
 * Filename:      obexftp_cli.c
 * Version:       0.7
 * Description:   Transfer from/to Siemens Mobile Equipment via OBEX
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 * Modified at:   Mon, 29 Apr 2002 22:58:53 +0100
 * Modified by:   Christian W. Zuckschwerdt <zany@triq.net>
 *
 *   Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void info_cb(int event, const char *msg, /*@unused@*/ int len, /*@unused@*/ void *data)
{
	switch (event) {

	case OBEXFTP_EV_ERRMSG:
		printf("Error: %s\n", msg);
		break;

	case OBEXFTP_EV_ERR:
		printf("failed: %s\n", msg);
		break;
	case OBEXFTP_EV_OK:
		printf("done\n");
		break;

	case OBEXFTP_EV_CONNECTING:
		printf("Connecting...");
		break;
	case OBEXFTP_EV_DISCONNECTING:
		printf("Disconnecting...");
		break;
	case OBEXFTP_EV_SENDING:
		printf("Sending %s...", msg);
		break;
	case OBEXFTP_EV_RECEIVING:
		printf("Receiving %s...", msg);
		break;

	case OBEXFTP_EV_LISTENING:
		printf("Waiting for incoming connection\n");
		break;

	case OBEXFTP_EV_CONNECTIND:
		printf("Incoming connection\n");
		break;
	case OBEXFTP_EV_DISCONNECTIND:
		printf("Disconnecting\n");
		break;

	case OBEXFTP_EV_INFO:
		printf("Got info %d: \n", (int)msg);
		break;

	}
}

/*@only@*/ /*@null@*/ static obexftp_client_t *cli = NULL;
/*@only@*/ /*@null@*/ static char *tty = NULL;
/*@only@*/ /*@null@*/ static char *transport = NULL;

static int cli_connect()
{
/*@only@*/ /*@null@*/ static obex_ctrans_t *ctrans = NULL;
	int retry;

	if (cli != NULL)
		return TRUE;

	if (tty != NULL) {
		if ((transport != NULL) && !strcasecmp(transport, "ericsson")) {
			printf("Custom transport set to 'Ericsson'\n");
			ctrans = cobex_pe_ctrans (tty);
		} else {
			ctrans = cobex_ctrans (tty);
			printf("Custom transport set to 'Siemens'\n");
		}
	}
	else {
		ctrans = NULL;
		printf("No custom transport\n");
	}

	/* Open */
	cli = obexftp_cli_open (info_cb, ctrans, NULL);
	if(cli == NULL) {
		printf("Error opening obexftp-client\n");
		return FALSE;
	}

	for (retry = 0; retry < 3; retry++) {

		/* Connect */
		if (obexftp_cli_connect (cli) >= 0)
			return TRUE;
		printf("Still trying to connect\n");
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
	int c;
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

	/* by default don't debug anything */
	/*
	log_handler = g_log_set_handler (NULL,
					 G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO,
					 g_log_null_handler, NULL);
	*/
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"device",	required_argument, NULL, 'd'},
			{"transport",	required_argument, NULL, 't'},
			{"list",	optional_argument, NULL, 'l'},
			{"chdir",	required_argument, NULL, 'c'},
			{"get",		required_argument, NULL, 'g'},
			{"put",		required_argument, NULL, 'p'},
			{"info",	no_argument, NULL, 'i'},
			{"move",	required_argument, NULL, 'm'},
			{"delete",	required_argument, NULL, 'k'},
			{"verbose",	no_argument, NULL, 'v'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'u'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-d:t:l::c:g:p:im:k:vh",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;

		switch (c) {
		
		case 'd':
			if (tty != NULL)
				free (tty);

			if (!strcasecmp(optarg, "irda"))
				tty = NULL;
			else
				tty = optarg;

			break;
		
		case 't':
			if (transport != NULL)
				free (transport);

			transport = optarg;

			break;

		case 'l':
			if(cli_connect ()) {
				/* List folder */
				(void) obexftp_list(cli, optarg, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'c':
			if(cli_connect ()) {
				/* Get file */
				(void) obexftp_setpath(cli, optarg, FALSE);
			}
			most_recent_cmd = c;
			break;

		case 'g':
			if(cli_connect ()) {
				/* Get file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				(void) obexftp_get(cli, optarg, p);
			}
			most_recent_cmd = c;
			break;

		case 'p':
			if(cli_connect ()) {
				/* Send file */
				(void) obexftp_put(cli, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'i':
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

		case 'k':
			if(cli_connect ()) {
				/* Delete file */
				(void) obexftp_del(cli, optarg);
			}
			most_recent_cmd = c;
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
				"Copyright (c) 2002 Christian W. Zuckschwerdt\n"
				"\n"
				" -d, --device <device>       connect to this device\n"
				" -t, --transport <type>      use a custom transport\n"
				" -l, --list [<FOLDER>]       list a folder\n"
				" -c, --chdir <DIR>           chdir / mkdir\n"
				" -g, --get <SOURCE>          fetch files\n"
				" -p, --put <SOURCE>          send files\n"
				" -i, --info                  retrieve misc infos\n\n"
				" -m, --move <SRC> <DEST>     move files\n"
				" -v, --verbose               verbose messages\n"
				" -k, --delete <SOURCE>       delete files\n"
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
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	cli_disconnect ();

	exit (0);

}
