/**
	\file apps/obexftp.c
	Command line client to transfer from/to Mobile Equipment via OBEX.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002-2007 Christian W. Zuckschwerdt <zany@triq.net>

	ObexFTP is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with ObexFTP. If not, see <http://www.gnu.org/>.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <sys/types.h>

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <obexftp/uuid.h>

#ifdef _WIN32
#define strcasestr strstr
#define ETIMEDOUT	WSAETIMEDOUT
#define ECONNREFUSED	WSAECONNREFUSED
#define EHOSTDOWN	WSAEHOSTDOWN
#define EINPROGRESS	WSAEINPROGRESS
#endif /* _WIN32 */

#include <common.h>

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
		// OBEX_EV_REQDONE: obex_rsp=43  (SE user reject)
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
		printf("Got info %u: \n", *(uint32_t*)msg);
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
static const char *fbs_uuid = (const char *)UUID_FBS;
static const char *irmc_uuid = (const char *)UUID_IRMC;
static const char *s45_uuid = (const char *)UUID_S45;
static const char *pcsoftware_uuid = (const char *)UUID_PCSOFTWARE;

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
	
        if (!strncasecmp(name, "pcsoftware", 10) || !strncasecmp(name, "sharp", 5)) {
		fprintf(stderr, "Using PCSOFTWARE uuid.\n");
		if (uuid) *uuid = pcsoftware_uuid;
		if (uuid_len) *uuid_len = sizeof(UUID_PCSOFTWARE);
		return sizeof(UUID_PCSOFTWARE);
	}

	return -1;
}

static void discover_usb()
{
	char **devices;
	char **dev;
	int interfaces_number;

       	devices = obexftp_discover(OBEX_TRANS_USB);
	interfaces_number = 0;
	for(dev = devices; *dev; dev++) interfaces_number++;
	printf("Found %d USB OBEX interfaces\n\n", interfaces_number);
       	for(dev = devices; *dev; dev++) {
		fprintf(stderr, "%s\n", *dev);
       	}
	printf("\nUse '-u interface_number' to connect\n");
}

static int find_bt(char *addr, char **res_bdaddr, int *res_channel)
{
	char **devices;
	char **dev;

	*res_bdaddr = addr;
	if (!addr || strlen(addr) < (6*2+5) || addr[2]!=':') {
  		fprintf(stderr, "Scanning for %s ...\n", addr);
		devices = obexftp_discover(OBEX_TRANS_BLUETOOTH);
  
		for(dev = devices; dev && *dev; dev++) {
      			if (!addr || strcasestr(*dev, addr)) {
				fprintf(stderr, "Found: %s\n", *dev);
				*res_bdaddr = *dev;
				break;
			}
       			fprintf(stderr, "Seen: %s\n", *dev);
		}
	}
	if (!*res_bdaddr)
		return -1; /* No (matching) BT device found */
//	(*res_bdaddr)[17] = '\0';
  
       	if (*res_channel < 0) {
		fprintf(stderr, "Browsing %s ...\n", *res_bdaddr);
		*res_channel = obexftp_browse_bt_ftp(*res_bdaddr);
	}
	if (*res_channel < 0)
		return -1; /* No valid BT channel found */

	return 0;
}


/*@only@*/ /*@null@*/ static obexftp_client_t *cli = NULL;
static const char *src_dev = NULL;
#ifdef HAVE_BLUETOOTH
static int transport = OBEX_TRANS_BLUETOOTH;
#else
static int transport = OBEX_TRANS_IRDA;
#endif /* HAVE_BLUETOOTH */
/*@only@*/ /*@null@*/ static char *device = NULL;
static int channel = -1;
static const char *use_uuid = (const char *)UUID_FBS;
static int use_uuid_len = sizeof(UUID_FBS);
static int use_conn=1;
static int use_path=1;


/* connect with given uuid. re-connect every time */
static int cli_connect_uuid(const char *uuid, int uuid_len)
{
	int ret, retry;
#ifdef HAVE_SYS_TIMES_H
	clock_t clock;
#endif

	if (cli == NULL) {

		/* Open */
		cli = obexftp_open (transport, NULL, info_cb, NULL);
		if(cli == NULL) {
			fprintf(stderr, "Error opening obexftp-client\n");
			exit(1);
			//return -1;
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
#ifdef HAVE_SYS_TIMES_H
		clock = times(NULL);
#endif
		ret = obexftp_connect_src(cli, src_dev, device, channel, uuid, uuid_len);
#ifdef HAVE_SYS_TIMES_H
		clock = times(NULL)-clock;
		fprintf(stderr, "Tried to connect for %ldms\n", clock);
#endif
		if (ret >= 0)
       			return ret;
		switch (errno) {
		
		case ETIMEDOUT:
			perror("The device may have moved out of range");
			break; /* retry */

		case ECONNREFUSED:
			perror("The user may have rejected the transfer");
			return -errno;

		case EHOSTDOWN:
			perror("The device may be out of range or turned off");
			break; /* retry */

		case EINPROGRESS:
			perror("Interupted/bad reception or the device moved out of range");
			break; /* retry */

		default:
			perror("error on connect()");
			break;
		} 

		fprintf(stderr, "Still trying to connect\n");
	}

	obexftp_close(cli);
	cli = NULL;
	return ret;
}

/* connect, possibly without fbs uuid. won't re-connect */
static int cli_connect()
{
	if (cli != NULL) {
		return 0;
	}

	/* complete bt address if necessary */
	if (transport == OBEX_TRANS_BLUETOOTH) {
		find_bt(device, &device, &channel);
		// we should free() the find_bt result at some point
	}
	if (cli_connect_uuid(use_uuid, use_uuid_len) < 0)
		exit(1);

	return 0;
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
	
	if (cli_connect_uuid(uuid, uuid_len) < 0) {
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


int main(int argc, char *argv[])
{
	int verbose = 0;
	int most_recent_cmd = 0;
	char *output_file = NULL;
	char *move_src = NULL;
	int ret = 0;

	/* preset mode of operation depending on our name */
	if (strstr(argv[0], "ls") != NULL)	most_recent_cmd = 'l';
	if (strstr(argv[0], "get") != NULL)	most_recent_cmd = 'g';
	if (strstr(argv[0], "put") != NULL)	most_recent_cmd = 'p';
	if (strstr(argv[0], "rm") != NULL)	most_recent_cmd = 'k';

	/* preset the port from environment */
	if (getenv(OBEXFTP_CHANNEL) != NULL) {
		channel = atoi(getenv(OBEXFTP_CHANNEL));
	}
	if (channel >= 0) {
		transport = OBEX_TRANS_USB;
		fprintf(stderr, "Presetting to USB: %d\n", channel);
	}
	if (getenv(OBEXFTP_CABLE) != NULL) {
		device = getenv(OBEXFTP_CABLE);
		transport = OBEX_TRANS_CUSTOM;
		fprintf(stderr, "Presetting to TTY: %s\n", device);
	}
	if (getenv(OBEXFTP_BLUETOOTH) != NULL) {
		device = getenv(OBEXFTP_BLUETOOTH);
		transport = OBEX_TRANS_BLUETOOTH;
		fprintf(stderr, "Presetting to BT: %s (%d)\n", device, channel);
	}
	if (getenv(OBEXFTP_INET) != NULL) {
		device = getenv(OBEXFTP_INET);
		transport = OBEX_TRANS_INET;
		fprintf(stderr, "Presetting to INET: %s\n", device);
	}
	       

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"irda",	no_argument, NULL, 'i'},
#ifdef HAVE_BLUETOOTH
			{"bluetooth",	optional_argument, NULL, 'b'},
			{"channel",	required_argument, NULL, 'B'},
			{"hci",		required_argument, NULL, 'd'},
#endif
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
		
		c = getopt_long (argc, argv, "-ib::B:d:u::t:n:U::HSL::l::c:C:f:o:g:G:p:k:XYxm:VvhN:FP",
				 long_options, &option_index);
		if (c == -1)
			break;
	
		if (c == 1)
			c = most_recent_cmd;
	
		switch (c) {
		
		case 'i':
			transport = OBEX_TRANS_IRDA;
       			device = NULL;
			channel = 0;
			break;
		
#ifdef HAVE_BLUETOOTH
		case 'b':
			transport = OBEX_TRANS_BLUETOOTH;
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
			}
       			device = optarg;
			break;
			
		case 'B':
			channel = atoi(optarg);
			break;
			
		case 'd':
			src_dev = optarg;
			break;
#endif /* HAVE_BLUETOOTH */

#ifdef HAVE_USB
		case 'u':
#ifndef _WIN32
			if (geteuid() != 0)
				fprintf(stderr, "If USB doesn't work setup permissions in udev or run as superuser.\n");
#endif
			transport = OBEX_TRANS_USB;
       			device = NULL;
			/* handle severed optional option argument */
			if (!optarg && argc > optind && argv[optind][0] != '-') {
				optarg = argv[optind];
				optind++;
				channel = atoi(optarg);
			} else {
				discover_usb();
			}
			break;
#endif /* HAVE_USB */
		
		case 't':
			transport = OBEX_TRANS_CUSTOM;
       			device = optarg;
			channel = 0;

			if (strstr(optarg, "ir") != NULL)
				fprintf(stderr, "Do you really want to use IrDA via ttys?\n");
			break;

		case 'N':
			fprintf(stderr,"Option -%c is deprecated, use -%c instead\n",'N','n');
		case 'n':
			transport = OBEX_TRANS_INET;
       			device = optarg;
			channel = 0;

			{
				int n;
				if (sscanf(optarg, "%d.%d.%d.%d", &n, &n, &n, &n) != 4)
					fprintf(stderr, "Please use dotted quad notation.\n");
			}
			break;
			
		case 'F':
			fprintf(stderr,"Option -%c is deprecated, use -%c instead\n",'F','U');
			optarg = "none";
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
			if (cli_connect() >= 0) {
				/* List folder */
				stat_entry_t *ent;
				void *dir = obexftp_opendir(cli, optarg);
				while ((ent = obexftp_readdir(dir)) != NULL) {
					stat_entry_t *st;
					st = obexftp_stat(cli, ent->name);
					if (!st) continue;
					printf("%d %s%s\n", st->size, ent->name,
						ent->mode&S_IFDIR?"/":"");
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
			if (cli_connect() >= 0) {
				/* List folder */
				ret = obexftp_list(cli, NULL, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'c':
			if (cli_connect() >= 0) {
				/* Change dir */
				ret = obexftp_setpath(cli, optarg, 0);
			}
			most_recent_cmd = c;
			break;

		case 'C':
			if (cli_connect() >= 0) {
				/* Change or Make dir */
				ret = obexftp_setpath(cli, optarg, 1);
			}
			most_recent_cmd = c;
			break;

		case 'o':
			output_file = optarg;
			break;

		case 'g':
		case 'G':
			if (cli_connect() >= 0) {
				char *p; /* basename or output_file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				if (output_file) p = output_file;
				/* Get file */
				if (obexftp_get(cli, p, optarg) && c == 'G')
					ret = obexftp_del(cli, optarg);
				output_file = NULL;
			}
			most_recent_cmd = c;
			break;

		case 'p':
			if (cli_connect() >= 0) {
				char *p; /* basename or output_file */
				if ((p = strrchr(optarg, '/')) != NULL) p++;
				else p = optarg;
				if (output_file) p = output_file;
				/* Send file */
				ret = obexftp_put_file(cli, optarg, p);
				output_file = NULL;
			}
			most_recent_cmd = c;
			break;

		case 'k':
			if (cli_connect() >= 0) {
				/* Delete file */
				ret = obexftp_del(cli, optarg);
			}
			most_recent_cmd = c;
			break;

		case 'X':
			if (cli_connect() >= 0) {
				/* Get capabilities */
				ret = obexftp_get_capability(cli, optarg, 0);
			}
			most_recent_cmd = 'h'; // not really
			break;

		case 'P':
			fprintf(stderr,"Option -%c is deprecated, use -%c instead\n",'P','Y');
		case 'Y':
			if (cli == NULL)
				probe_device();
			fprintf(stderr, "No other transfer options allowed with --probe\n");
			break;

		case 'x':
			if (cli_connect() >= 0) {
				/* for S65 */
				(void) obexftp_disconnect (cli);
				(void) obexftp_connect_uuid (cli, device, channel, UUID_S45, sizeof(UUID_S45));
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
			if (cli_connect() >= 0) {
				/* Rename a file */
				ret = obexftp_rename(cli, move_src, optarg);
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
				" [ -d, --hci <no/address> ]  use source device with this address or number\n"
#endif
#ifdef HAVE_USB
				" -u, --usb [<intf>]          connect to a usb interface or list interfaces\n"
#endif
				" -t, --tty <device>          connect to this tty using a custom transport\n"
				" -n, --network <host>        connect to this host\n\n"
				" -U, --uuid                  use given uuid (none, FBS, IRMC, S45, SHARP)\n"
				" -H, --noconn                suppress connection ids (no conn header)\n"
				" -S, --nopath                dont use setpaths (use path as filename)\n\n"
				" -c, --chdir <DIR>           chdir\n"
				" -C, --mkdir <DIR>           mkdir and chdir\n"
				" -l, --list [<FOLDER>]       list current/given folder\n"
				" -o, --output <PATH>         specify the target file name\n"
				"                             get and put always specify the remote name.\n"
				" -g, --get <SOURCE>          fetch files\n"
				" -G, --getdelete <SOURCE>    fetch and delete (move) files \n"
				" -p, --put <SOURCE>          send files\n"
				" -k, --delete <SOURCE>       delete files\n\n"
				" -X, --capability            retrieve capability object\n"
				" -Y, --probe                 probe and report device characteristics\n"
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
		if (ret < 0)
			printf("The operation failed with return code %d\n", -ret);
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

	exit (-ret);

}
