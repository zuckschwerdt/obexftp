#include <glib.h>
#include <string.h>

#include "debug.h"
#include "ircp.h"
#include "ircp_client.h"
#include "ircp_server.h"

#include "cobex_S45.h"


//
//
//
void ircp_info_cb(gint event, gchar *param)
{
	DEBUG(4, G_GNUC_FUNCTION "()\n");
	switch (event) {

	case IRCP_EV_ERRMSG:
		g_print("Error: %s\n", param);
		break;

	case IRCP_EV_ERR:
		g_print("failed: %s\n", param);
		break;
	case IRCP_EV_OK:
		g_print("done\n");
		break;


	case IRCP_EV_CONNECTING:
		g_print("Connecting...");
		break;
	case IRCP_EV_DISCONNECTING:
		g_print("Disconnecting...");
		break;
	case IRCP_EV_SENDING:
		g_print("Sending %s...", param);
		break;
	case IRCP_EV_RECEIVING:
		g_print("Receiving %s...", param);
		break;

	case IRCP_EV_LISTENING:
		g_print("Waiting for incoming connection\n");
		break;

	case IRCP_EV_CONNECTIND:
		g_print("Incoming connection\n");
		break;
	case IRCP_EV_DISCONNECTIND:
		g_print("Disconnecting\n");
		break;



	}
}

//
//
//
int main(int argc, char *argv[])
{
	int i;
	ircp_client_t *cli;
	ircp_server_t *srv;
	gchar *inbox;
        obex_ctrans_t *ctrans = NULL;

	gchar *p;

        p = strchr(argv[0], '/') + 1;
        if( strcmp(p, "cable") == 0 ) {
                ctrans = cobex_ctrans();
	}

	if(argc >= 2 && strcmp(argv[1], "-r") == 0) {
		srv = ircp_srv_open(ircp_info_cb);
		if(srv == NULL) {
			g_print("Error opening ircp-server\n");
			return -1;
		}

		if(argc >= 3)
			inbox = argv[2];
		else
			inbox = ".";

		ircp_srv_recv(srv, inbox);
#ifdef DEBUG_TCP
		sleep(2);
#endif
		ircp_srv_close(srv);
	}
		

	else if(argc == 1) {
		g_print("Usage: %s file1, file2, ...\n"
			"  or:  %s -l [FOLDER]\n"
			"  or:  %s -g [SOURCE]\n"
			"  or:  %s -m [SOURCE] [DEST]\n"
			"  or:  %s -d [SOURCE]\n"
			"  or:  %s -r [DEST]\n"
			"  or:  %s -i\n\n"
			"Send files over IR. Use -l to list a folder,\n"
			"use -g to fetch files, use -m to move files,\n"
			"use -d to delete files, use -r to receive files.\n"
			"use -i to retrieve misc infos.\n",
			argv[0], argv[0], argv[0], argv[0],
			argv[0], argv[0], argv[0]);
		return 0;
	}
	else if(strcmp(argv[1], "-i") == 0) {
		cli = ircp_cli_open(ircp_info_cb, ctrans);
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) >= 0) {
			// Retrieve Info
			ircp_info(cli, 0x01);
			ircp_info(cli, 0x02);

			// Disconnect
			ircp_cli_disconnect(cli);
		}
		ircp_cli_close(cli);
	}
	else if(strcmp(argv[1], "-l") == 0) {
		cli = ircp_cli_open(ircp_info_cb, ctrans);
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) >= 0) {
			// Send all files
			for(i = 2; i < argc; i++) {
				ircp_list(cli, argv[i], argv[i]);
			}

			// Disconnect
			ircp_cli_disconnect(cli);
		}
		ircp_cli_close(cli);
	}
	else if(strcmp(argv[1], "-g") == 0) {
		cli = ircp_cli_open(ircp_info_cb, ctrans);
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) >= 0) {
			// Send all files
			for(i = 2; i < argc; i++) {
				ircp_get(cli, argv[i], argv[i]);
			}

			// Disconnect
			ircp_cli_disconnect(cli);
		}
		ircp_cli_close(cli);
	}
	else if(strcmp(argv[1], "-m") == 0) {
		cli = ircp_cli_open(ircp_info_cb, ctrans);
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) >= 0) {
			// Rename a file
			ircp_rename(cli, argv[2], argv[3]);

			// Disconnect
			ircp_cli_disconnect(cli);
		}
		ircp_cli_close(cli);
	}
	else if(strcmp(argv[1], "-d") == 0) {
		cli = ircp_cli_open(ircp_info_cb, ctrans);
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) >= 0) {
			// Delete all files
			for(i = 2; i < argc; i++) {
				ircp_del(cli, argv[i]);
			}

			// Disconnect
			ircp_cli_disconnect(cli);
		}
		ircp_cli_close(cli);
	}
	else {
		cli = ircp_cli_open(ircp_info_cb, ctrans);
		if(cli == NULL) {
			g_print("Error opening ircp-client\n");
			return -1;
		}
			
		// Connect
		if(ircp_cli_connect(cli) >= 0) {
			// Send all files
			for(i = 1; i < argc; i++) {
				ircp_put(cli, argv[i]);
			}

			// Disconnect
			ircp_cli_disconnect(cli);
		}
		ircp_cli_close(cli);
	}
	return 0;
}
