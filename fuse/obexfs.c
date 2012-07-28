/*
 *  obexfs.c: FUSE Filesystem to access OBEX
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


typedef struct data_buffer data_buffer_t;
struct data_buffer {
	size_t size;
	char *data;
	int write_mode; /* is this a write buffer? */
};


static obexftp_client_t *cli = NULL;
static int transport = 0;
static char *source = NULL; // "00:11:22:33:44:55"; "hci0";
static char *device = NULL; // "00:11:22:33:44:55"; "/dev/ttyS0";
static int channel = -1;

static int nonblock = 0;

static char *mknod_dummy = NULL; /* bad coder, no cookies! */

static int nodal = 0;


static int cli_open()
{
        int retry;

        if (cli != NULL)
                return 0;

        /* Open */
        cli = obexftp_open (transport, NULL, NULL, NULL);
        if(cli == NULL) {
                /* Error opening obexftp-client */
                return -1;
        }

	if (channel < 0) {
		channel = obexftp_browse_bt_ftp(device);
	}

        for (retry = 0; retry < 3; retry++) {

                /* Connect */
                if (obexftp_connect_src (cli, source, device, channel, UUID_FBS, sizeof(UUID_FBS)) >= 0)
                        return 0;
                /* Still trying to connect */
		sleep(1);
        }

        cli = NULL;
        return -1;
}

static void cli_close()
{
        if (cli != NULL) {
                /* Disconnect */
                (void) obexftp_disconnect (cli);
                /* Close */
                obexftp_close (cli);
        }
	cli = NULL;
}

static int ofs_connect()
{
	if (!cli)
		return -1;
	if (nonblock) {
		if (++nodal > 1) {
			nodal--;
			return -EBUSY;
		}
	} else {
		while (++nodal > 1) {
			nodal--;
			sleep(1);
		}
	}
	DEBUG("%s() >>>blocking<<<\n", __func__);
	return 0;
}

static void ofs_disconnect()
{
	nodal--;
	DEBUG("%s() <<<unblocking>>>\n", __func__);
}

static int ofs_getattr(const char *path, struct stat *stbuf)
{
	stat_entry_t *st;
	int res;

	if(!path || *path == '\0' || !strcmp(path, "/")) {
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

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */
	
	st = obexftp_stat(cli, path);

	ofs_disconnect();
	
	if (!st)
		return -ENOENT;

	stbuf->st_mode = st->mode;
	stbuf->st_nlink = 1; /* should be NSUB+2 for  dirs */
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

static int ofs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	DIR *dir;
	stat_entry_t *ent;
	int res;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	dir = obexftp_opendir(cli, path);
	
	if (!dir) {
		ofs_disconnect();
		return -ENOENT;
	}

	res = filler(h, ".", DT_DIR, 0);
	res = filler(h, "..", DT_DIR, 0);
	while ((ent = obexftp_readdir(dir)) != NULL) {
		DEBUG("GETDIR:%s\n", ent->name);
		res = filler(h, ent->name, S_ISDIR(ent->mode) ? DT_DIR : DT_REG, 0);
		if(res != 0)
			break;
	}
	obexftp_closedir(dir);

	ofs_disconnect();

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
	int res;

	if(!path || *path != '/')
		return 0;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	(void) obexftp_setpath(cli, path, 1);

	ofs_disconnect();

	return 0;
}

static int ofs_unlink(const char *path)
{
	int res;

	if(!path || *path != '/')
		return 0;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	(void) obexftp_del(cli, path);

	ofs_disconnect();

	return 0;
}


static int ofs_rename(const char *from, const char *to)
{
	int res;

	if(!from || *from != '/')
		return 0;

	if(!to || *to != '/')
		return 0;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	(void) obexftp_rename(cli, from, to);

	ofs_disconnect();

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
	data_buffer_t *wb;
	int res = 0;
	size_t actual;

	if(!path || *path != '/')
		return 0;

	wb = (data_buffer_t *)fi->fh;
	if (!wb->data) {

		res = ofs_connect();
		if(res < 0)
			return res; /* errno */

		(void) obexftp_get(cli, NULL, path);
		wb->size = cli->buf_size;
		wb->data = cli->buf_data; /* would be better to memcpy this */
		//cli->buf_data = NULL; /* now the data is ours -- without copying */

		ofs_disconnect();
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
	data_buffer_t *wb;
	int res;
	
	wb = (data_buffer_t *)fi->fh;
	DEBUG("Releasing: %s (%p)\n", path, wb);
	if (wb && wb->data && wb->write_mode) {
		DEBUG("Now writing %s for %zu (%02x)\n", path, wb->size, wb->data[0]);

		res = ofs_connect();
		if(res < 0)
			return res; /* errno */

		(void) obexftp_put_data(cli, (uint8_t *)wb->data, wb->size, path);

		ofs_disconnect();

		free(wb->data);
		free(wb);
	}

	return 0;
}

#ifdef SIEMENS
static int ofs_statfs(const char *UNUSED(label), struct statfs *st)
{
	int res;
	int size, free;

	memset(st, 0, sizeof(struct statfs));

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	/* for S45 */
	(void) obexftp_disconnect (cli);
	(void) obexftp_connect_uuid (cli, device, channel, UUID_S45, sizeof(UUID_S45));
 
	/* Retrieve Infos */
	(void) obexftp_info(cli, 0x01);
	size = cli->apparam_info;
	(void) obexftp_info(cli, 0x02);
	free = cli->apparam_info;
 
 DEBUG("%s() GOT FS STAT: %d / %d\n", __func__, free, size);
 
	(void) obexftp_disconnect (cli);
	(void) obexftp_connect (cli, device, channel);

	ofs_disconnect();

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
	//cli_open();
	return NULL;
}

static void ofs_destroy(void *UNUSED(private_data)) {
	fprintf(stderr, "terminating...\n");
	cli_close();
	return;
}

static struct fuse_operations ofs_oper = {
	getattr:	ofs_getattr,
	readlink:	NULL,
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
	int res;
	
	while (1) {
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"irda",	no_argument, NULL, 'i'},
			{"bluetooth",	required_argument, NULL, 'b'},
			{"channel",	required_argument, NULL, 'B'},
			{"hci",	required_argument, NULL, 'd'},
			{"usb",		required_argument, NULL, 'u'},
			{"tty",		required_argument, NULL, 't'},
			{"network",	required_argument, NULL, 'n'},
			{"nonblock",	no_argument, NULL, 'N'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'h'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "+ib:B:d:u:t:n:Nh",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		
		case 'i':
			transport = OBEX_TRANS_IRDA;
			device = NULL;
			channel = 0;
			break;
		
		case 'b':
			transport = OBEX_TRANS_BLUETOOTH;
			device = optarg;
			channel = -1;
			break;
			
		case 'B':
			channel = atoi(optarg);
			break;

		case 'd':
			source = optarg;
			break;
			
		case 'u':
			if (geteuid() != 0)
				fprintf(stderr, "If USB doesn't work setup permissions in udev or run as superuser.\n");
			transport = OBEX_TRANS_USB;
			device = NULL;
			channel = atoi(optarg);
			break;
		
		case 't':
			transport = OBEX_TRANS_CUSTOM;
			device = optarg;
			channel = 0;
			break;
			
		case 'n':
			transport = OBEX_TRANS_INET;
			device = optarg;
			channel = 650;
			{
                                int n;
                                if (sscanf(optarg, "%d.%d.%d.%d", &n, &n, &n, &n) != 4)
                                        fprintf(stderr, "Please use dotted quad notation.\n");
                        }
			break;
			
		case 'N':
			nonblock = 1;
			break;

		case 'h':
			/* printf("ObexFS %s\n", VERSION); */
			printf("Usage: %s [-i | -b <dev> [-B <chan>] [-d <hci>] | -u <dev> | -t <dev> | -n <dev>] [-- <fuse options>]\n"
				"Transfer files from/to Mobile Equipment.\n"
				"Copyright (c) 2002-2005 Christian W. Zuckschwerdt\n"
				"\n"
				" -i, --irda                  connect using IrDA transport\n"
				" -b, --bluetooth <device>    connect to this bluetooth device\n"
				" -B, --channel <number>      use this bluetooth channel when connecting\n"
				" -d, --hci <no/address>      use source device with this address or number\n"
				" -u, --usb <interface>       connect to this usb interface number\n"
				" -t, --tty <device>          connect to this tty using a custom transport\n"
				" -n, --network <device>      connect to this network host\n\n"
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

	if (transport == 0) {
	       	fprintf(stderr, "No device selected. Use --help for help.\n");
		exit(0);
	}

	argv[optind-1] = argv[0];

        /* Open connection */
	res = cli_open();
	if(res < 0)
		return res; /* errno */
	
	/* loop */
	fuse_main(argc-optind+1, &argv[optind-1], &ofs_oper);
	
        /* Close connection */
	cli_close();

	return 0;
}
