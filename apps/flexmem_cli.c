/*
 *                
 * Filename:      obexftp_cli.c
 * Version:       0.6
 * Description:   Transfer from/to Siemens Mobile Equipment via OBEX
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Don, 17 Jan 2002 18:27:25 +0100
 * Modified at:   Don,  7 Feb 2002 12:24:55 +0100
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

void info_cb(gint event, const gchar *msg, gint len, gpointer data)
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

obexftp_client_t *cli = NULL;
obex_ctrans_t *ctrans = NULL;

gboolean cli_connect()
{
	if (cli != NULL)
		return TRUE;

	/* Open */
	cli = obexftp_cli_open (info_cb, ctrans, NULL);
	if(cli == NULL) {
		g_print("Error opening obexftp-client\n");
		return FALSE;
	}

	/* Connect */
	if (obexftp_cli_connect (cli) >= 0)
		return TRUE;

	cli = NULL;
	return FALSE;
}

void cli_disconnect()
{
	if (cli != NULL) {
		/* Disconnect */
		obexftp_cli_disconnect (cli);
		/* Close */
		obexftp_cli_close (cli);
	}
}

int main(int argc, char *argv[])
{
	int c;
	int most_recent_cmd = 0;
	gchar *move_src = NULL;
	gchar *inbox;
	gchar *tty = NULL;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"device", 1, 0, 'd'},
			{"list", 2, 0, 'l'},
			{"chdir", 1, 0, 'c'},
			{"get", 1, 0, 'g'},
			{"put", 1, 0, 'p'},
			{"info", 0, 0, 'i'},
			{"move", 1, 0, 'm'},
			{"delete", 1, 0, 'k'},
			{"receive", 0, 0, 'r'},
			{"help", 0, 0, 'h'},
			{"usage", 0, 0, 'u'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-d:l::c:g:p:im:k:rh",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;

		switch (c) {
		
		case 'd':
			if (!strcasecmp(optarg, "irda"))
				tty = NULL;
			else
				tty = optarg;

			if (tty != NULL)
				ctrans = cobex_ctrans (tty);
			else
				ctrans = NULL;

			break;

		case 'l':
			if(cli_connect ()) {
				/* List folder */
				obexftp_list(cli, optarg, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'c':
			if(cli_connect ()) {
				/* Get file */
				obexftp_setpath(cli, optarg, FALSE);
			}
			most_recent_cmd = c;
			break;

		case 'g':
			if(cli_connect ()) {
				/* Get file */
				obexftp_get(cli, optarg, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'p':
			if(cli_connect ()) {
				/* Send file */
				obexftp_put(cli, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'i':
			if(cli_connect ()) {
				/* Retrieve Infos */
				obexftp_info(cli, 0x01);
				obexftp_info(cli, 0x02);
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
				obexftp_rename(cli, move_src, optarg);
			}
			move_src = NULL;
			break;

		case 'k':
			if(cli_connect ()) {
				/* Delete file */
				obexftp_del(cli, optarg);
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
				"                             fallback to $MOBILEPHONE_DEV\n"
				"                             then /dev/mobilephone and /dev/ttyS0\n"
				" -l, --list [<FOLDER>]       list a folder\n"
				" -c, --chdir <DIR>           chdir / mkdir\n"
				" -g, --get <SOURCE>          fetch files\n"
				" -p, --put <SOURCE>          send files\n"
				" -i, --info                  retrieve misc infos\n\n"
				" -m, --move <SRC> <DEST>     move files\n"
				" -k, --delete <SOURCE>       delete files\n"
				" -r, --receive [<DEST>       receive files\n"
				" -h, --help, --usage         this help text\n"
				"\n",
				argv[0]);
			break;

		default:
			g_print ("Try `%s --help' for more information.\n",
				 argv[0]);
		}
	}

	if (optind < argc) {
		g_print ("non-option ARGV-elements: ");
		while (optind < argc)
			g_print ("%s ", argv[optind++]);
		g_print ("\n");
	}

	cli_disconnect ();

	exit (0);

}
