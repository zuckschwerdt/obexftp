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

#include <glib.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <cobexbfb/cobex_bfb.h>
#include <cobexpe/cobex_pe.h>

static void info_cb(gint event, const gchar *msg, /*@unused@*/ gint len, /*@unused@*/ gpointer data)
{
	switch (event) {

	case OBEXFTP_EV_ERRMSG:
		g_print("Error: %s\n", msg);
		break;

	case OBEXFTP_EV_ERR:
		g_print("failed: %s\n", msg);
		break;
	case OBEXFTP_EV_OK:
		g_print("done\n");
		break;

	case OBEXFTP_EV_CONNECTING:
		g_print("Connecting...");
		break;
	case OBEXFTP_EV_DISCONNECTING:
		g_print("Disconnecting...");
		break;
	case OBEXFTP_EV_SENDING:
		g_print("Sending %s...", msg);
		break;
	case OBEXFTP_EV_RECEIVING:
		g_print("Receiving %s...", msg);
		break;

	case OBEXFTP_EV_LISTENING:
		g_print("Waiting for incoming connection\n");
		break;

	case OBEXFTP_EV_CONNECTIND:
		g_print("Incoming connection\n");
		break;
	case OBEXFTP_EV_DISCONNECTIND:
		g_print("Disconnecting\n");
		break;

	case OBEXFTP_EV_INFO:
		g_print("Got info %d: \n", GPOINTER_TO_UINT (msg));
		break;

	}
}

/*@only@*/ /*@null@*/ static obexftp_client_t *cli = NULL;
/*@only@*/ /*@null@*/ static gchar *tty = NULL;
/*@only@*/ /*@null@*/ static gchar *transport = NULL;

static gboolean cli_connect()
{
/*@only@*/ /*@null@*/ static obex_ctrans_t *ctrans = NULL;
	int retry;

	if (cli != NULL)
		return TRUE;

	if (tty != NULL) {
		if ((transport != NULL) && !strcasecmp(transport, "ericsson")) {
			g_print("Custom transport set to 'Ericsson'\n");
			ctrans = cobex_pe_ctrans (tty);
		} else {
			ctrans = cobex_ctrans (tty);
			g_print("Custom transport set to 'Siemens'\n");
		}
	}
	else {
		ctrans = NULL;
		g_print("No custom transport\n");
	}

	/* Open */
	cli = obexftp_cli_open (info_cb, ctrans, NULL);
	if(cli == NULL) {
		g_print("Error opening obexftp-client\n");
		return FALSE;
	}

	for (retry = 0; retry < 3; retry++) {

		/* Connect */
		if (obexftp_cli_connect (cli) >= 0)
			return TRUE;
		g_print("Still trying to connect\n");
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
	int c;
	int most_recent_cmd = 0;
	gchar *p;
	gchar *move_src = NULL;
	/* gchar *inbox; */

	/* preset mode of operation depending on our name */
	if (strstr(argv[0], "ls") != NULL)	most_recent_cmd = 'l';
	if (strstr(argv[0], "get") != NULL)	most_recent_cmd = 'g';
	if (strstr(argv[0], "put") != NULL)	most_recent_cmd = 'p';
	if (strstr(argv[0], "mv") != NULL)	most_recent_cmd = 'm';
	if (strstr(argv[0], "rm") != NULL)	most_recent_cmd = 'k';

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
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'u'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-d:t:l::c:g:p:im:k:h",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;

		switch (c) {
		
		case 'd':
			if (tty != NULL)
				g_free (tty);

			if (!strcasecmp(optarg, "irda"))
				tty = NULL;
			else
				tty = optarg;

			break;
		
		case 't':
			if (transport != NULL)
				g_free (transport);

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

		case 'h':
		case 'u':
			g_print("Usage: %s [-d <dev>] [-s|-a] [-l <dir> ...] [-c <dir>]\n"
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
				" -k, --delete <SOURCE>       delete files\n"
				" -h, --help, --usage         this help text\n"
				"\n",
				argv[0]);
			break;

		default:
			g_print("Try `%s --help' for more information.\n",
				 argv[0]);
		}
	}

	if (optind < argc) {
		g_print("non-option ARGV-elements: ");
		while (optind < argc)
			g_print("%s ", argv[optind++]);
		g_print("\n");
	}

	cli_disconnect ();

	exit (0);

}
