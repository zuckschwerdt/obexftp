/*
 *  obexautofs.c: FUSE Filesystem to access OBEX with automount
 *  This is just a wrapper. ObexFTP API does the real work.
 *
 *  Copyright (c) 2003-2006 Christian W. Zuckschwerdt <zany@triq.net>
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

/* strndup */
#define _GNU_SOURCE

/* at least fuse v 2.2 is needed */
#define FUSE_USE_VERSION 22
#include <fuse.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <getopt.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <obexftp/uuid.h>

#define UNUSED(x) x __attribute__((unused))

#define DEBUGOUPUT
#ifdef DEBUGOUPUT
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) do { } while (0)
#endif


typedef struct connection connection_t;
struct connection {
	char *alias;
	int transport;
	char *addr;
	int channel;
	obexftp_client_t *cli;
	int nodal;
	int recent;
	connection_t *next;
};

#define SCAN_INTERVAL	2
static time_t last_scan = 0;
static connection_t *connections;


static char *tty = NULL; // "/dev/ttyS0";
static char *source = NULL; // "00:11:22:33:44:55"; "hci0";
static int search_irda = 1;
static int search_bt = 1;
static int search_usb = 1;
static int nonblock = 0;


static int discover_irda(void) { return -1; }

static int discover_usb(void) {
	char **devices, **dev;
	char name[25], *p;
	connection_t *conn;

	DEBUG("Scanning USB...\n");
	devices = obexftp_discover(OBEX_TRANS_USB);

	for(dev = devices; *dev; dev++) {
		strcpy(name, "usb");
		strncpy(&name[3], *dev, sizeof(name)-4);
		p = strchr(name, ' ');
		if (p)
			*p = '\0';
       		DEBUG("Found %s (%d), %s\n", name, atoi(*dev), *dev);

		for (conn = connections; conn; conn = conn->next) {
	       		if (!strcmp(conn->addr, name)) {
				conn->recent++;
				break;
			}
		}
	
		if (!conn) {
			DEBUG("Adding %s\n", name);
			conn = calloc(1, sizeof(connection_t));
			if (!conn)
				return -1;
			conn->alias = NULL;
			conn->transport = OBEX_TRANS_USB;
			conn->addr = strdup(name);
			conn->channel = atoi(*dev);
			//conn->cli = cli_open(OBEX_TRANS_USB, NULL, conn->channel);
       			conn->recent++;
			conn->next = connections;
			connections = conn;
		}
	}
	return 0;
}

static int discover_tty(char *UNUSED(port)) { return -1; }

static int discover_bt(void)
{
	char *hci = NULL;
	char **devs, **dev, *name;
	connection_t *conn;

	DEBUG("Scanning ...\n");
	devs = obexftp_discover_bt_src(hci);

	if(!devs) {
		perror("Inquiry failed.");
		return -1;
	}
  
	for(dev = devs; dev && *dev; dev++) {
		for (conn = connections; conn; conn = conn->next) {
	       		if (!strcmp(conn->addr, *dev)) {
				conn->recent++;
				break;
			}
		}
	
		if (!conn) {
			name = obexftp_bt_name_src(*dev, hci);
			DEBUG("Adding\t%s\t%s\n", *dev, name);
			conn = calloc(1, sizeof(connection_t));
			if (!conn)
				return -1;
			conn->alias = name;
			conn->transport = OBEX_TRANS_BLUETOOTH;
			conn->addr = *dev;
			conn->channel = obexftp_browse_bt_ftp(conn->addr);
			//conn->cli = cli_open(OBEX_TRANS_BLUETOOTH, conn->addr, conn->channel);
       			conn->recent++;
			conn->next = connections;
			connections = conn;
		} else
			free(*dev);
	}
  
	free(devs);
	return 0;
}


static void cli_close(obexftp_client_t *cli);

static int discover_devices(void) {
	connection_t *conn, *prev;

       	for (conn = connections; conn; conn = conn->next)
		conn->recent = 0;

        if (search_irda)
		discover_irda();
        if (search_bt)
		discover_bt();
        if (search_usb)
		discover_usb();
        if (tty)
		discover_tty(tty);
	
	/* remove from head */
	while (connections && connections->recent == 0) {
		DEBUG("Deleting %s\n", connections->addr);
       		conn = connections;
       		connections = conn->next;
       	
       		cli_close(conn->cli);
		if (conn->alias)
	       		free(conn->alias);
       		free(conn->addr);
       		free(conn);
       	}

	/* remove from body */
	for (prev = connections; prev; prev = prev->next)
		if(prev->next && prev->next->recent == 0) {
		DEBUG("Deleting2 %s\n", prev->next->addr);
	       		conn = prev->next;
       			prev->next = conn->next;
       	
       			cli_close(conn->cli);
			if (conn->alias)
	       			free(conn->alias);
       			free(conn->addr);
	       		free(conn);
		}
	
	return 0;
}
 

typedef struct data_buffer data_buffer_t;
struct data_buffer {
	size_t size;
	char *data;
	int write_mode; /* is this a write buffer? */
};


static char *mknod_dummy = NULL; /* bad coder, no biscuits! */


/* connection handling operations */

static obexftp_client_t *cli_open(int transport, char *addr, int channel)
{
	obexftp_client_t *cli;
        int retry;

        /* Open */
        cli = obexftp_open (transport, NULL, NULL, NULL);
        if(cli == NULL) {
                /* Error opening obexftp-client */
                return NULL;
        }

        for (retry = 0; retry < 3; retry++) {

                /* Connect */
                if (obexftp_connect_src (cli, source, addr, channel, UUID_FBS, sizeof(UUID_FBS)) >= 0)
                        return cli;
                /* Still trying to connect */
		sleep(1);
        }

        return NULL;
}

static void cli_close(obexftp_client_t *cli)
{
	if (!cli)
		return;
	/* Disconnect */
	(void) obexftp_disconnect (cli);
	/* Close */
	obexftp_close (cli);
}

static int ofs_connect(connection_t *conn)
{
	if (!conn || !conn->cli)
		return -ENOENT;
	if (nonblock) {
		if (++conn->nodal > 1) {
			conn->nodal--;
			return -EBUSY;
		}
	} else {
		while (++conn->nodal > 1) {
			conn->nodal--;
			sleep(1);
		}
	}
	DEBUG("%s() >>>blocking<<<\n", __func__);
	return 0;
}

static void ofs_disconnect(connection_t *conn)
{
	if (!conn || !conn->cli)
		return; /* -ENOENT */
	conn->nodal--;
	DEBUG("%s() <<<unblocking>>>\n", __func__);
}

static connection_t *ofs_find_connection(const char *path, char **filepath)
{
	int namelen;
	connection_t *conn;

        if (!path || path[0] != '/') {
		DEBUG("Invalid base path \"%s\"\n", path);
		return NULL;
	}
	
	path++;
        *filepath = strchr(path, '/');
	if (*filepath)
		namelen = *filepath - path;
	else
		namelen = strlen(path);
	
	for (conn = connections; conn; conn = conn->next) {
        	if (!strncmp(conn->addr, path, namelen) || (conn->alias && !strncmp(conn->alias, path, namelen))) {
			if (!conn->cli)
				conn->cli = cli_open(conn->transport, conn->addr, conn->channel);
			return conn;
		}
	}
       	return NULL;
}
 

/* file and directory operations */

static int ofs_getattr(const char *path, struct stat *stbuf)
{
	connection_t *conn;
	char *filepath;

	stat_entry_t *st;
	int res;

	if(!path || *path == '\0' || !strcmp(path, "/")) {
		/* root */
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 1; /* should be NSUB+2 for dirs */
		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();
		stbuf->st_size = 0;
		stbuf->st_blocks = 0;
		stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
		return 0;
	}
	
	DEBUG("%s() '%s'\n", __func__, path);

	if (mknod_dummy && !strcmp(path, mknod_dummy)) {
		/* fresh mknod dummy */
		stbuf->st_mode = S_IFREG | 0755;
		stbuf->st_nlink = 1; /* should be NSUB+2 for dirs */
		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();
		stbuf->st_size = 0;
		stbuf->st_blocks = 0;
		stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
		free(mknod_dummy);
		mknod_dummy = NULL;
		return 0;
	}

        conn = ofs_find_connection(path, &filepath);
	if (!conn)
		return -ENOENT;
	
	if(!filepath) {
		/* the device entry itself */
		if (!strcmp(path + 1, conn->addr))
			stbuf->st_mode = S_IFDIR | 0755;
		else
			stbuf->st_mode = S_IFLNK | 0777;
		stbuf->st_nlink = 1; /* should be NSUB+2 for dirs */
		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();
		stbuf->st_size = 0;
		stbuf->st_blocks = 0;
		stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
		return 0;
	}
	
	res = ofs_connect(conn);
	if(res < 0)
		return res; /* errno */
	
	st = obexftp_stat(conn->cli, filepath);

	ofs_disconnect(conn);
	
	if (!st)
		return -ENOENT;

	stbuf->st_mode = st->mode;
	stbuf->st_nlink = 1; /* should be NSUB+2 for dirs */
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	stbuf->st_size = st->size;
	stbuf->st_blksize = 512; /* they expect us to do so... */
	stbuf->st_blocks = (st->size + stbuf->st_blksize) / stbuf->st_blksize;
	stbuf->st_mtime = st->mtime;
	stbuf->st_atime = st->mtime;
	stbuf->st_ctime = st->mtime;

	return 0;
}

static int ofs_readlink (const char *path, char *link, size_t UNUSED(size))
{
	connection_t *conn;

	for (conn = connections; conn; conn = conn->next) {
		if(conn->alias && !strcmp(conn->alias, path + 1)) {
			strcpy(link, conn->addr);
			return 0;
		}
	}
	return -ENOENT;
}

static int ofs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	connection_t *conn;
	char *filepath;
	
	DIR *dir;
	stat_entry_t *ent;
	struct stat stat;
	int res;
	
	if(!path || *path == '\0' || !strcmp(path, "/")) {
		/* list devices */
		if (last_scan + SCAN_INTERVAL < time(NULL)) {
			discover_devices();
			last_scan = time(NULL);
		}
	
		DEBUG("listing devices...\n");
		res = filler(h, ".", DT_DIR, 0);
		res = filler(h, "..", DT_DIR, 0);
		for (conn = connections; conn; conn = conn->next) {
			stat.st_mode = DT_DIR;
			if (conn->alias)
				res = filler(h, conn->alias, DT_LNK, 0);
			res = filler(h, conn->addr, DT_DIR, 0);
			if(res != 0)
				break;
		}
		return 0;
	}

        conn = ofs_find_connection(path, &filepath);
	if (!conn)
		return -1; /* FIXME */
	
	res = ofs_connect(conn);
	if(res < 0)
		return res; /* errno */

	dir = obexftp_opendir(conn->cli, filepath);
	
	if (!dir) {
		ofs_disconnect(conn);
		return -ENOENT;
	}

	res = filler(h, ".", DT_DIR, 0);
	res = filler(h, "..", DT_DIR, 0);
	while ((ent = obexftp_readdir(dir)) != NULL) {
		DEBUG("GETDIR:%s\n", ent->name);
		stat.st_mode = S_ISDIR(ent->mode) ? DT_DIR : DT_REG;
		res = filler(h, ent->name, S_ISDIR(ent->mode) ? DT_DIR : DT_REG, 0);
		if(res != 0)
			break;
	}
	obexftp_closedir(dir);

	ofs_disconnect(conn);

	return 0;
}

/* needed for creating files and writing to them */
static int ofs_mknod(const char *path, mode_t UNUSED(mode), dev_t UNUSED(dev))
{
	/* check for access */
	
	/* create dummy for subsequent stat */
	if (mknod_dummy) {
		free(mknod_dummy);
		fprintf(stderr, "warning: overlapping mknod calls.\n");
	}
	mknod_dummy = strdup(path);

	return 0;
}

static int ofs_mkdir(const char *path, mode_t UNUSED(mode))
{
	connection_t *conn;
	char *filepath;
	int res;

	if(!path || *path != '/')
		return 0;

        conn = ofs_find_connection(path, &filepath);
	if (!conn)
		return -1; /* FIXME */

	res = ofs_connect(conn);
	if(res < 0)
		return res; /* errno */

	(void) obexftp_setpath(conn->cli, filepath, 1);

	ofs_disconnect(conn);

	return 0;
}

static int ofs_unlink(const char *path)
{
	connection_t *conn;
	char *filepath;
	int res;

	if(!path || *path != '/')
		return 0;

        conn = ofs_find_connection(path, &filepath);
	if (!conn)
		return -1; /* FIXME */

	res = ofs_connect(conn);
	if(res < 0)
		return res; /* errno */

	(void) obexftp_del(conn->cli, filepath);

	ofs_disconnect(conn);

	return 0;
}


static int ofs_rename(const char *from, const char *to)
{
	connection_t *conn;
	char *filepath;
	int res;

	if(!from || *from != '/')
		return 0;

	if(!to || *to != '/')
		return 0;

        conn = ofs_find_connection(from, &filepath);
	if (!conn)
		return -1; /* FIXME */

	res = ofs_connect(conn);
	if(res < 0)
		return res; /* errno */

	(void) obexftp_rename(conn->cli, from, to);

	ofs_disconnect(conn);

	return 0;
}

/* needed for overwriting files */
static int ofs_truncate(const char *UNUSED(path), off_t UNUSED(offset))
{
	DEBUG("%s() called. This is a dummy!\n", __func__);
	return 0;
}

/* well RWX for everyone I guess! */
static int ofs_open(const char *UNUSED(path), struct fuse_file_info *fi)
{
	data_buffer_t *wb;
	
	wb = calloc(1, sizeof(data_buffer_t));
	if (!wb)
		return -1;
	fi->fh = (unsigned long)wb;
	
    return 0;
}

static int ofs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *UNUSED(fi))
{
	connection_t *conn;
	char *filepath;
	data_buffer_t *wb;
	int res = 0;
	size_t actual;

	if(!path || *path != '/')
		return 0;

	wb = (data_buffer_t *)fi->fh;
	if (!wb->data) {

        	conn = ofs_find_connection(path, &filepath);
		if (!conn)
			return -1; /* FIXME */

		res = ofs_connect(conn);
		if(res < 0)
			return res; /* errno */

		(void) obexftp_get(conn->cli, NULL, filepath);
		wb->size = conn->cli->buf_size;
		wb->data = conn->cli->buf_data; /* would be better to memcpy this */
		//conn->cli->buf_data = NULL; /* now the data is ours -- without copying */

		ofs_disconnect(conn);
	}
	actual = wb->size - offset;
	if (actual > size)
		actual = size;
	DEBUG("reading %s at %" PRId64 " for %zu (peek: %02x\n", path, offset, actual, wb->data[offset]);
	memcpy(buf, wb->data + offset, actual);

	return actual;
}

static int ofs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	data_buffer_t *wb;
	size_t newsize;
	DEBUG("Writing %s at %" PRId64 " for %zu\n", path, offset, size);
	wb = (data_buffer_t *)fi->fh;

	if (!wb)
		return -1;
	
	if (offset + size > wb->size)
		newsize = offset + size;
	else
		newsize = wb->size; /* don't change the buffer size */
	
	if (!wb->data)
		wb->data = malloc(newsize);
	else if (newsize != wb->size)
		wb->data = realloc(wb->data, newsize);
	if (!wb->data)
		return -1;
	wb->size = newsize;
	wb->write_mode = 1;

	DEBUG("memcpy to %p (%p) from %p cnt %zu\n", wb->data + offset, wb->data, buf, size);
	(void) memcpy(&wb->data[offset], buf, size);

	return size;
}

/* careful, this can be a read release or a write release */
static int ofs_release(const char *path, struct fuse_file_info *fi)
{
	connection_t *conn;
	char *filepath;
	data_buffer_t *wb;
	int res;
	
	wb = (data_buffer_t *)fi->fh;
	DEBUG("Releasing: %s (%p)\n", path, wb);
	if (wb && wb->data && wb->write_mode) {
		DEBUG("Now writing %s for %zu (%02x)\n", path, wb->size, wb->data[0]);

	        conn = ofs_find_connection(path, &filepath);
		if (!conn)
			return -1; /* FIXME */

		res = ofs_connect(conn);
		if(res < 0)
			return res; /* errno */

		(void) obexftp_put_data(conn->cli, (uint8_t*)wb->data, wb->size, filepath);

		ofs_disconnect(conn);

		free(wb->data);
		free(wb);
	}

	return 0;
}

#ifdef SIEMENS
/* just sum all clients */
static int ofs_statfs(const char *UNUSED(label), struct statfs *st)
{
	connection_t *conn;
	int size = 0, free = 0;

        for (conn = connections; conn; conn = conn->next)
		if (conn->cli && ofs_connect(conn) >= 0) {

			/* for S45 */
			(void) obexftp_disconnect (conn->cli);
			(void) obexftp_connect_uuid (conn->cli, conn->addr, conn->channel, UUID_S45, sizeof(UUID_S45));
 
			/* Retrieve Infos */
			(void) obexftp_info(conn->cli, 0x01);
			size += conn->cli->apparam_info;
			(void) obexftp_info(conn->cli, 0x02);
			free += conn->cli->apparam_info;
 
			 DEBUG("%s() GOT FS STAT: %d / %d\n", __func__, free, size);
 
			(void) obexftp_disconnect (conn->cli);
			(void) obexftp_connect (conn->cli, conn->addr, conn->channel);

			ofs_disconnect(conn);
		}

	memset(st, 0, sizeof(struct statfs));
	st->f_bsize = 1;	/* optimal transfer block size */
	st->f_blocks = size;	/* total data blocks in file system */
	st->f_bfree = free;	/* free blocks in fs */
	st->f_bavail = free;	/* free blocks avail to non-superuser */

	/* st->f_files;		/ * total file nodes in file system */
	/* st->f_ffree;		/ * free file nodes in fs */
	/* st->f_namelen;	/ * maximum length of filenames */

	return 0;
}
#endif

static void *ofs_init(void) {

        /* Open connection */
	//res = cli_open();
	//if(res < 0)
	//	return res; /* errno */

       	//discover_bt(&alias, &bdaddr, &channel);
	//cli_open();
	return NULL;
}

static void ofs_destroy(void *UNUSED(private_data)) {
	connection_t *conn;
	
	DEBUG("terminating...\n");
        /* Close connection */
	for (conn = connections; conn; conn = conn->next) {
		cli_close(conn->cli);
		if (conn->alias)
       			free(conn->alias);
       		free(conn->addr);
       		free(conn);
	}
	return;
}


/* main */

static struct fuse_operations ofs_oper = {
	getattr:	ofs_getattr,
	readlink:	ofs_readlink,
	opendir:	NULL,
	readdir:	NULL,
	releasedir:	NULL,
	getdir:		ofs_getdir,
	mknod:		ofs_mknod,
	mkdir:		ofs_mkdir,
	symlink:	NULL,
	unlink:		ofs_unlink,
	rmdir:		ofs_unlink,
	rename:		ofs_rename,
	link:		NULL,
	chmod:		NULL,
	chown:		NULL,
	truncate:	ofs_truncate,
	utime:		NULL,
	open:		ofs_open,
	read:		ofs_read,
	write:		ofs_write,
#ifdef SIEMENS
	statfs:		ofs_statfs,
#endif
	release:	ofs_release,
	flush:		NULL,
	fsync:		NULL,
	init:		ofs_init,
	destroy:	ofs_destroy
};

int main(int argc, char *argv[])
{
	while (1) {
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"noirda",	no_argument, NULL, 'I'},
			{"nobluetooth",	no_argument, NULL, 'B'},
			{"nousb",	no_argument, NULL, 'U'},
			{"tty",		required_argument, NULL, 't'},
			{"hci", required_argument, NULL, 'd'},
			{"nonblock",	no_argument, NULL, 'N'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'h'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "+IBUt:d:Nh",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		
		case 'I':
			search_irda = 0;
			break;
		
		case 'B':
       			search_bt = 0;
			break;
			
		case 'U':
			search_usb = 0;
			break;
		
		case 't':
			if (tty != NULL)
				free (tty);
       			tty = NULL;

			if (strcasecmp(optarg, "irda"))
				tty = optarg;
			break;

		case 'd':
			source = optarg;
			break;
			
		case 'N':
			nonblock = 1;
			break;

		case 'h':
			/* printf("ObexFS %s\n", VERSION); */
			printf("Usage: %s [-I] [-B] [-U] [-t <dev>] [-d <hci>] [-N] [-- <fuse options>]\n"
				"Transfer files from/to Mobile Equipment.\n"
				"Copyright (c) 2002-2005 Christian W. Zuckschwerdt\n"
				"\n"
				" -I, --noirda                dont search for IrDA devices\n"
				" -B, --nobluetooth           dont search for bluetooth devices\n"
				" -U, --nousb                 dont search for usb devices\n"
				" -t, --tty <device>          search for devices at this tty\n\n"
				" -d, --hci <no/address>      use only this source device address or number\n"				
				" -N, --nonblock              nonblocking mode\n\n"
				" -h, --help, --usage         this help text\n\n"
				"Options to fusermount need to be preceeded by two dashes (--).\n"
				"\n",
				argv[0]);
			exit(0);
			break;

		default:
			printf("Try `%s --help' for more information.\n",
				 argv[0]);
			exit(0);
		}
	}

	if (!search_irda && !search_bt && !search_usb && !tty) {
	       	fprintf(stderr, "No device selected. Use --help for help.\n");
		exit(0);
	}

	argv[optind-1] = argv[0];
	
	fprintf(stderr, "IrDA searching not available.\n");
	fprintf(stderr, "USB searching not available.\n");
	fprintf(stderr, "TTY searching not available.\n");
	/* loop */
	fuse_main(argc-optind+1, &argv[optind-1], &ofs_oper);

	return 0;
}
