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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <cobexbfb/cobex_bfb.h>

#include <common.h>

#ifdef _WIN32
/* OpenOBEX won't define a handler on win32 */
void DEBUG(unsigned int n, ...) { }
void DUMPBUFFER(unsigned int n, char *label, char *msg) { }
#endif /* _WIN32 */

static void info_cb(int event, const char *msg, int len, void *data)
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


int main(int argc, char *argv[])
{
	obexftp_client_t *cli = NULL;
	obex_ctrans_t *ctrans = NULL;
	char *tty = NULL;
	int i;

	tty = "/dev/ttyS0";
	ctrans = cobex_ctrans (tty);

	for (i = 0; i < 20; i++) {

		/* Open */
		cli = obexftp_cli_open (info_cb, ctrans, NULL);
		if(cli == NULL) {
			printf("Error opening obexftp-client\n");
			exit (-1);
		}

		/* Connect */
		if (obexftp_cli_connect (cli) < 0) {
			printf("Error connecting to obexftp-client\n");
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
