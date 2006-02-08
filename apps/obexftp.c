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
#include <errno.h>

#include <sys/types.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <obexftp/uuid.h>

#ifdef _WIN32_FIXME
/* OpenOBEX won't define a handler on win32 */
void DEBUG(unsigned int n, ...) { }
void DUMPBUFFER(unsigned int n, char *label, char *msg) { }
#endif /* _WIN32 */

#include <common.h>

//#ifdef HAVE_BLUETOOTH
#include "bt_discovery.h"
//#endif

// perhaps this scheme would be better?
// IRDA		irda://[nick?]
// CABLE	tty://path
// BLUETOOTH	bt://[device[:channel]]
// USB		usb://[enum]
// INET		host://host[:port]
#define OBEXFTP_CABLE "OBEXFTP_CABLE"
#define OBEXFTP_BLUETOOTH "OBEXFTP_BLUETOOTH"
#define OBEXFTP_USB "OBEXFTP_USB"
#define OBEXFTP_INET "OBEXFTP_INET"
#define OBEXFTP_CHANNEL "OBEXFTP_CHANNEL"

/* current command, set by main, read from info_cb */
int c;

static void info_cb(int event, const char *msg, int len, void *UNUSED(data))
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
		fprintf(stderr, "Sending \"%s\"... ", msg);
		break;
	case OBEXFTP_EV_RECEIVING:
		fprintf(stderr, "Receiving \"%s\"... ", msg);
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
		printf("Got info %d: \n", (int)msg); // 64 bit problems ?
		break;

	case OBEXFTP_EV_BODY:
		if (c == 'l' || c == 'X' || c == 'P') {
			if (msg == NULL)
				fprintf(stderr, "No body.\n");
			else if (len == 0)
				fprintf(stderr, "Empty body.\n");
			else
				write(STDOUT_FILENO, msg, len);
		}
		break;

	case OBEXFTP_EV_PROGRESS:
		fprintf(stderr, "%c%c", 0x08, progress[i++]);
		fflush(stdout);
		if (i >= strlen(progress))
			i = 0;
		break;

	}
}

/* create global uuid buffers */
static const char *fbs_uuid = UUID_FBS;
static const char *irmc_uuid = UUID_IRMC;
static const char *s45_uuid = UUID_S45;

/* parse UUID string to real bytes */
static int parse_uuid(char *name, const char **uuid, int *uuid_len)
{
	if (name == NULL || *name == '\0' ||
			!strncasecmp(name, "none", 4) ||
			!strncasecmp(name, "null", 4) ||
			!strncasecmp(name, "push", 4) ||
			!strncasecmp(name, "goep", 4)) {
		fprintf(stderr, "Suppressing FBS.\n");
		if (uuid) *uuid = NULL;
		if (uuid_len) *uuid_len = 0;
		return 0;
	}

        if (!strncasecmp(name, "fbs", 3) || !strncasecmp(name, "ftp", 3)) {
		fprintf(stderr, "Using FBS uuid.\n");
		if (uuid) *uuid = fbs_uuid;
		if (uuid_len) *uuid_len = sizeof(UUID_FBS);
		return sizeof(UUID_FBS);
	}

        if (!strncasecmp(name, "sync", 4) || !strncasecmp(name, "irmc", 4)) {
		fprintf(stderr, "Using SYNCH uuid.\n");
		if (uuid) *uuid = irmc_uuid;
		if (uuid_len) *uuid_len = sizeof(UUID_IRMC);
		return sizeof(UUID_IRMC);
	}

        if (!strncasecmp(name, "s45", 3) || !strncasecmp(name, "sie", 3)) {
		fprintf(stderr, "Using S45 uuid.\n");
		if (uuid) *uuid = s45_uuid;
		if (uuid_len) *uuid_len = sizeof(UUID_S45);
		return sizeof(UUID_S45);
	}

	return -1;
}

/*@only@*/ /*@null@*/ static obexftp_client_t *cli = NULL;
#ifdef HAVE_BLUETOOTH
static int transport = OBEX_TRANS_BLUETOOTH;
#else
static int transport = OBEX_TRANS_IRDA;
#endif /* HAVE_BLUETOOTH */
/*@only@*/ /*@null@*/ static char *tty = NULL;
/*@only@*/ /*@null@*/ static char *btaddr = NULL;
static int btchannel = -1;
static int usbinterface = -1;
/*@only@*/ /*@null@*/ static char *inethost = NULL;
static const char *use_uuid = UUID_FBS;
static int use_uuid_len = sizeof(UUID_FBS);
static int use_conn=1;
static int use_path=1;


/* connect with given uuid. re-connect every time */
static int cli_connect_uuid(const char *uuid, int uuid_len)
{
	int retry;

	if (cli == NULL) {

		/* Open */
		cli = obexftp_open (transport, NULL, info_cb, NULL);
		if(cli == NULL) {
			fprintf(stderr, "Error opening obexftp-client\n");
			exit(1);
			//return FALSE;
		}
		if (!use_conn) {
			cli->quirks &= ~OBEXFTP_CONN_HEADER;
		}
		if (!use_path) {
			cli->quirks &= ~OBEXFTP_SPLIT_SETPATH;
		}
	}	
	for (retry = 0; retry < 3; retry++) {

		/* Connect */
                switch (transport) {
		case OBEX_TRANS_INET:
			if (obexftp_connect_uuid (cli, inethost, 0, uuid, uuid_len) >= 0)
				return TRUE;
			break;
		case OBEX_TRANS_IRDA:
			if (obexftp_connect_uuid (cli, NULL, 0, uuid, uuid_len) >= 0)
				return TRUE;
			break;
		case OBEX_TRANS_USB:
			if (obexftp_connect_uuid (cli, NULL, usbinterface, uuid, uuid_len) >= 0)
				return TRUE;
			break;
		case OBEX_TRANS_CUSTOM:
			if (obexftp_connect_uuid (cli, tty, 0, uuid, uuid_len) >= 0)
				return TRUE;
			break;
		case OBEX_TRANS_BLUETOOTH:
			if (obexftp_connect_uuid (cli, btaddr, btchannel, uuid, uuid_len) >= 0)
				return TRUE;
			break;
		default:
			fprintf(stderr, "Transport type unknown\n");
			return FALSE;
		}
		fprintf(stderr, "Still trying to connect\n");
	}

	cli = NULL;
	return FALSE;
}

/* connect, possibly without fbs uuid. won't re-connect */
static int cli_connect()
{
	if (cli != NULL) {
		return TRUE;
	}

	if (!cli_connect_uuid(use_uuid, use_uuid_len))
		exit(1);

	return TRUE;
}

static void cli_disconnect()
{
	if (cli != NULL) {
		/* Disconnect */
		(void) obexftp_disconnect (cli);
		/* Close */
		obexftp_close (cli);
		cli = NULL;
	}
}

static void probe_device_uuid(const char *uuid, int uuid_len)
{
	int rsp[10];
	
	if (!cli_connect_uuid(uuid, uuid_len)) {
		printf("couldn't connect.\n");
		return;
	}
	
	printf("getting null object without type\n");
       	(void) obexftp_get_type(cli, NULL, NULL, NULL);
	rsp[0] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	printf("getting empty object without type\n");
       	(void) obexftp_get_type(cli, NULL, NULL, "");
	rsp[1] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	
	printf("getting null object with x-obex/folder-listing type\n");
       	(void) obexftp_get_type(cli, XOBEX_LISTING, NULL, NULL);
	rsp[2] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	printf("getting empty object with x-obex/folder-listing type\n");
       	(void) obexftp_get_type(cli, XOBEX_LISTING, NULL, "");
	rsp[3] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);


	printf("getting null object with x-obex/capability type\n");
       	(void) obexftp_get_type(cli, XOBEX_CAPABILITY, NULL, NULL);
	rsp[4] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	printf("getting empty object with x-obex/capability type\n");
       	(void) obexftp_get_type(cli, XOBEX_CAPABILITY, NULL, "");
	rsp[5] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	
	printf("getting null object with x-obex/object-profile type\n");
       	(void) obexftp_get_type(cli, XOBEX_PROFILE, NULL, NULL);
	rsp[6] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	printf("getting empty object with x-obex/object-profile type\n");
       	(void) obexftp_get_type(cli, XOBEX_PROFILE, NULL, "");
	rsp[7] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

		
	printf("getting telecom/devinfo.txt object\n");
	cli->quirks = 0;
       	(void) obexftp_get_type(cli, NULL, NULL, "telecom/devinfo.txt");
	rsp[8] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);

	printf("getting telecom/devinfo.txt object with setpath\n");
	cli->quirks = (OBEXFTP_LEADING_SLASH | OBEXFTP_TRAILING_SLASH | OBEXFTP_SPLIT_SETPATH);
       	(void) obexftp_get_type(cli, NULL, NULL, "telecom/devinfo.txt");
	rsp[9] = cli->obex_rsp;
	printf("response code %02x\n", cli->obex_rsp);
	
	printf("=== response codes === %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		rsp[0], rsp[1], rsp[2], rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

	//cli_disconnect();
	if (cli != NULL) {
		(void) obexftp_disconnect (cli);
	}
		
}

/* try the whole probing with different uuids */
static void probe_device()
{
	printf("\n=== Probing with FBS uuid.\n");
	probe_device_uuid(UUID_FBS, sizeof(UUID_FBS));
	
	printf("\n=== Probing with S45 uuid.\n");
	probe_device_uuid(UUID_S45, sizeof(UUID_S45));
	
	printf("\n=== Probing without uuid.\n");
	probe_device_uuid(NULL, 0);
	
	printf("\nEnd of probe.\n");
	exit (0);
}


#ifdef HAVE_USB
static void discover_cb(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
	(void) handle;
	(void) object;
	(void) mode;
	(void) event;
	(void) obex_cmd;
	(void) obex_rsp;
}

static void discover_usb()
{
	obex_t *handle;
	obex_interface_t* obex_intf;
	int i, interfaces_number;

	if(! (handle = OBEX_Init(OBEX_TRANS_USB, discover_cb, 0))) {
		printf( "OBEX_Init failed\n");
		return;
	}
	interfaces_number = OBEX_FindInterfaces(handle, &obex_intf);
	printf("Found %d USB OBEX interfaces\n", interfaces_number);
	for (i=0; i < interfaces_number; i++)
		printf("Interface %d:\n\tManufacturer: %s\n\tProduct: %s\n\tInterface description: %s\n", i,
			obex_intf[i].usb.manufacturer,
			obex_intf[i].usb.product,
		       	obex_intf[i].usb.control_interface);
	printf("Use '-u interface_number' to connect\n");
	OBEX_Cleanup(handle);
}
#endif /* HAVE_USB */


int main(int argc, char *argv[])
{
	int verbose=0;
	int most_recent_cmd = 0;
	char *target_path = NULL;
	char *output_file = NULL;
	char *move_src = NULL;
	/* char *inbox; */

	/* preset mode of operation depending on our name */
	if (strstr(argv[0], "ls") != NULL)	most_recent_cmd = 'l';
	if (strstr(argv[0], "get") != NULL)	most_recent_cmd = 'g';
	if (strstr(argv[0], "put") != NULL)	most_recent_cmd = 'p';
	if (strstr(argv[0], "rm") != NULL)	most_recent_cmd = 'k';

	/* preset the port from environment */
	if (getenv(OBEXFTP_CHANNEL) != NULL) {
		btchannel = atoi(getenv(OBEXFTP_CHANNEL));
		usbinterface = atoi(getenv(OBEXFTP_CHANNEL));
	}
	if (usbinterface >= 0) {
		transport = OBEX_TRANS_USB;
		fprintf(stderr, "Using USB: %d\n", usbinterface);
	}
	tty = getenv(OBEXFTP_CABLE);
	if (tty != NULL) {
		tty = strdup(tty);
		transport = OBEX_TRANS_CUSTOM;
		fprintf(stderr, "Using TTY: %s\n", tty);
	}
	btaddr = getenv(OBEXFTP_BLUETOOTH);
	if (btaddr != NULL) {
		if (btchannel <= 0 || strlen(btaddr) < (6*2+5) || btaddr[2]!=':')
			discover_bt(btaddr, &btaddr, &btchannel);
		else
			btaddr = strdup(btaddr);
		transport = OBEX_TRANS_BLUETOOTH;
		fprintf(stderr, "Using BT: %s (%d)\n", btaddr, btchannel);
	}
	inethost = getenv(OBEXFTP_INET);
	if (inethost != NULL) {
		inethost = strdup(inethost);
		transport = OBEX_TRANS_INET;
		fprintf(stderr, "Using INET: %s\n", inethost);
	}
	       

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"irda",	no_argument, NULL, 'i'},
#ifdef HAVE_BLUETOOTH
			{"bluetooth",	optional_argument, NULL, 'b'},
#endif
			{"channel",	required_argument, NULL, 'B'},
#ifdef HAVE_USB
			{"usb",		optional_argument, NULL, 'u'},
#endif
			{"tty",		required_argument, NULL, 't'},
			{"network",	required_argument, NULL, 'n'},
			{"uuid",	optional_argument, NULL, 'U'},
			{"noconn",	no_argument, NULL, 'H'},
			{"nopath",	no_argument, NULL, 'S'},
			{"list",	optional_argument, NULL, 'l'},
			{"chdir",	required_argument, NULL, 'c'},
			{"mkdir",	required_argument, NULL, 'C'},
			{"path",	required_argument, NULL, 'f'},
			{"output",	required_argument, NULL, 'o'},
			{"get",		required_argument, NULL, 'g'},
			{"getdelete",	required_argument, NULL, 'G'},
			{"put",		required_argument, NULL, 'p'},
			{"delete",	required_argument, NULL, 'k'},
			{"capability",	no_argument, NULL, 'X'},
			{"probe",	no_argument, NULL, 'Y'},
			{"info",	no_argument, NULL, 'x'},
			{"move",	required_argument, NULL, 'm'},
			{"verbose",	no_argument, NULL, 'v'},
			{"version",	no_argument, NULL, 'V'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'h'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-ib::B:u::t:n:U::HSL::l::c:C:f:o:g:G:p:k:XYxm:Vvh",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;
	
		switch (c) {
		
		case 'i':
			transport = OBEX_TRANS_IRDA;
			break;
		
#ifdef HAVE_BLUETOOTH
		case 'b':
			transport = OBEX_TRANS_BLUETOOTH;
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

#ifdef HAVE_USB
		case 'u':
			if (geteuid() != 0)
				fprintf(stderr, "Superuser privileges are required to access USB.\n");
			transport = OBEX_TRANS_USB;
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
				usbinterface = atoi(optarg);
			} else {
				discover_usb();
			}
			break;
#endif /* HAVE_USB */
		
		case 't':
			transport = OBEX_TRANS_CUSTOM;
			if (tty != NULL)
				free (tty); /* ok to to free an optarg? */
       			tty = optarg; /* strdup? */

			if (strstr(optarg, "ir") != NULL)
				fprintf(stderr, "Do you really want to use IrDA via ttys?\n");
			break;

		case 'n':
			transport = OBEX_TRANS_INET;
			if (inethost != NULL)
				free (inethost); /* ok to to free an optarg? */
       			inethost = optarg; /* strdup? */

			{
				int n;
				if (sscanf(optarg, "%d.%d.%d.%d", &n, &n, &n, &n) != 4)
					fprintf(stderr, "Please use dotted quad notation.\n");
			}
			break;
			
		case 'U':
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
			}
			if (parse_uuid(optarg, &use_uuid, &use_uuid_len) < 0)
				fprintf(stderr, "Unknown UUID %s\n", optarg);
			break;

		case 'H':
			use_conn=0;
			break;

		case 'S':
			use_path=0;
			break;

		case 'L':
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
			}
			if(cli_connect ()) {
				/* List folder */
				stat_entry_t *ent;
				void *dir = obexftp_opendir(cli, optarg);
				while ((ent = obexftp_readdir(dir)) != NULL) {
					stat_entry_t *st;
					st = obexftp_stat(cli, ent->name);
					if (!st) continue;
					printf("%d %s (%d)\n", st->size, ent->name, ent->mode);
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

		case 'o':
			output_file = optarg;
			break;

		case 'g':
		case 'G':
			if(cli_connect ()) {
				char *p; /* basename or output_file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				if (output_file) p = output_file;
				/* Get file */
				if (obexftp_get(cli, p, optarg))
					(void) obexftp_del(cli, optarg);
				output_file = NULL;
			}
			most_recent_cmd = c;
			break;

		case 'p':
			if(cli_connect ()) {
				char *p; /* basename or output_file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				if (output_file) p = output_file;
				/* Send file */
				(void) obexftp_put_file(cli, optarg, p);
				output_file = NULL;
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

		case 'X':
			if(cli_connect ()) {
				/* Get capabilities */
				(void) obexftp_get_capability(cli, optarg, 0);
			}
			most_recent_cmd = 'h'; // not really
			break;

		case 'Y':
			if (cli == NULL)
				probe_device();
			fprintf(stderr, "No other transfer options allowed with --probe\n");
			break;

		case 'x':
			if(cli_connect ()) {
				/* for S65 */
				(void) obexftp_disconnect (cli);
				(void) obexftp_connect_uuid (cli, btaddr, btchannel, UUID_S45, sizeof(UUID_S45));
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
			printf("ObexFTP %s\n", VERSION);
			most_recent_cmd = 'h'; // not really
			break;

		case 'h':
			printf("ObexFTP %s\n", VERSION);
			printf("Usage: %s [ -i | -b <dev> [-B <chan>] | -U <intf> | -t <dev> | -N <host> ]\n"
				"[-c <dir> ...] [-C <dir> ] [-l [<dir>]]\n"
				"[-g <file> ...] [-p <files> ...] [-k <files> ...] [-x] [-m <src> <dest> ...]\n"
				"Transfer files from/to Mobile Equipment.\n"
				"Copyright (c) 2002-2004 Christian W. Zuckschwerdt\n"
				"\n"
				" -i, --irda                  connect using IrDA transport (default)\n"
#ifdef HAVE_BLUETOOTH
				" -b, --bluetooth [<device>]  use or search a bluetooth device\n"
				" [ -B, --channel <number> ]  use this bluetooth channel when connecting\n"
#endif
#ifdef HAVE_USB
				" -u, --usb [<intf>]          connect to a usb interface or list interfaces\n"
#endif
				" -t, --tty <device>          connect to this tty using a custom transport\n"
				" -n, --network <host>        connect to this host\n\n"
				" -U, --uuid                  use given uuid (none, FBS, IRMC, S45)\n"
				" -H, --noconn                suppress connection ids (no conn header)\n"
				" -S, --nopath                dont use setpaths (use path as filename)\n\n"
				" -c, --chdir <DIR>           chdir\n"
				" -C, --mkdir <DIR>           mkdir and chdir\n"
				" -l, --list [<FOLDER>]       list current/given folder\n"
				// " -f, --path <PATH>           specify the local file name or directory\n"
				"                             get and put always specify the remote name.\n"
				" -g, --get <SOURCE>          fetch files\n"
				" -G, --getdelete <SOURCE>    fetch and delete (move) files \n"
				" -p, --put <SOURCE>          send files\n"
				" -k, --delete <SOURCE>       delete files\n\n"
				" -X, --capability            retrieve capability object\n"
				" -P, --probe                 probe and report device characteristics\n"
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
