/*
 *                
 * Filename:      stress.c
 * Version:       0.1
 * Description:   Stress the cobex_bfb connections to Siemens Mobile Equipment
 * Status:        Experimental.
 * Author:        Christian W. Zuckschwerdt <zany@triq.net>
 * Created at:    Son, 17 Feb 2002 12:38:06 +0100
 * Modified at:   Son, 17 Feb 2002 12:38:06 +0100
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

#include "libobexftp/obexftp.h"
#include "libobexftp/client.h"
#include "libcobexbfb/cobex_bfb.h"

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


int main(int argc, char *argv[])
{
	obexftp_client_t *cli = NULL;
	obex_ctrans_t *ctrans = NULL;
	gchar *tty = NULL;
	gint i;

	tty = "/dev/ttyS0";
	ctrans = cobex_ctrans (tty);

	for (i = 0; i < 20; i++) {

		/* Open */
		cli = obexftp_cli_open (info_cb, ctrans, NULL);
		if(cli == NULL) {
			g_print("Error opening obexftp-client\n");
			exit (-1);
		}

		/* Connect */
		if (obexftp_cli_connect (cli) < 0) {
			g_print("Error connecting to obexftp-client\n");
			exit (-2);
		}

		/* Retrieve Infos */
		obexftp_info(cli, 0x01);
		obexftp_info(cli, 0x02);

		/* Retrieve Infos */
		obexftp_list(cli, NULL, NULL);

		if (cli != NULL) {

			/* Disconnect */
			obexftp_cli_disconnect (cli);
		
			/* Close */
			obexftp_cli_close (cli);
		}
	}
	exit (0);

}
