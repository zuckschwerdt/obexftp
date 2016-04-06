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
#include <stdbool.h>
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
#include <expat.h>

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
	bool write_mode; /* is this a write buffer? */
};


static obexftp_client_t *cli = NULL;
static int transport = 0;
static char *source = NULL; // "00:11:22:33:44:55"; "hci0";
static char *device = NULL; // "00:11:22:33:44:55"; "/dev/ttyS0";
static int channel = -1;
static char *root = NULL; // "E:";
static int root_len = 0; // Length of 'root'.

static bool nonblock = false;

// Free/total space (in bytes) to report if can't get real info.
static int report_space = 0;

static char *mknod_dummy = NULL; /* bad coder, no cookies! */

static int nodal = 0;

static char* translate_path(const char* path) {
	char* tpath = calloc(sizeof(char), root_len + strlen(path) + 1);
	if (root)
		strcpy(tpath, root);
	strcpy(tpath + root_len, path);
	DEBUG("TRANSLATED: %s\n", tpath);
	return tpath;
}

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
#ifdef SIEMENS_S45
                if (obexftp_connect_src (cli, source, device, channel, UUID_S45, sizeof(UUID_S45)) >= 0)
#else
                if (obexftp_connect_src (cli, source, device, channel, UUID_FBS, sizeof(UUID_FBS)) >= 0)
#endif
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
	char *tpath;
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
	
	tpath = translate_path(path);
	st = obexftp_stat(cli, tpath);
	free(tpath);

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
	char *tpath;
	DIR *dir;
	stat_entry_t *ent;
	int res;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	tpath = translate_path(path);
	dir = obexftp_opendir(cli, tpath);
	
	if (!dir) {
		free(tpath);
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
	free(tpath);

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
	char *tpath;
	int res;

	if(!path || *path != '/')
		return 0;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	tpath = translate_path(path);
	(void) obexftp_setpath(cli, tpath, 1);
	free(tpath);

	ofs_disconnect();

	return 0;
}

static int ofs_unlink(const char *path)
{
	char *tpath;
	int res;

	if(!path || *path != '/')
		return 0;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	tpath = translate_path(path);
	(void) obexftp_del(cli, tpath);
	free(tpath);

	ofs_disconnect();

	return 0;
}


static int ofs_rename(const char *from, const char *to)
{
	char *tfrom, *tto;
	int res;

	if(!from || *from != '/')
		return 0;

	if(!to || *to != '/')
		return 0;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

	tfrom = translate_path(from);
	tto = translate_path(to);
	(void) obexftp_rename(cli, tfrom, tto);
	free(tto);
	free(tfrom);

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
	char *tpath;
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

		tpath = translate_path(path);
		(void) obexftp_get(cli, NULL, tpath);
		free(tpath);

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
	wb->write_mode = true;

	DEBUG("memcpy to %p (%p) from %p cnt %zu\n", wb->data + offset, wb->data, buf, size);
	(void) memcpy(&wb->data[offset], buf, size);

	return size;
}

/* careful, this can be a read release or a write release */
static int ofs_release(const char *path, struct fuse_file_info *fi)
{
	char *tpath;
	data_buffer_t *wb;
	int res;
	
	wb = (data_buffer_t *)fi->fh;
	DEBUG("Releasing: %s (%p)\n", path, wb);
	if (wb && wb->data && wb->write_mode) {
		DEBUG("Now writing %s for %zu (%02x)\n", path, wb->size, wb->data[0]);

		res = ofs_connect();
		if(res < 0)
			return res; /* errno */

		tpath = translate_path(path);
		(void) obexftp_put_data(cli, (uint8_t *)wb->data, wb->size, tpath);
		free(tpath);

		ofs_disconnect();

		free(wb->data);
		free(wb);
	}

	return 0;
}

struct parsing_data {
	/* Input */
	const char *path;

	/* Intermediate states */
	enum state_t {
		PS_ROOT = 0,
		PS_CAPABILITY,
		PS_GENERAL,
		PS_MEMORY,
		PS_MEM_TYPE,
		PS_LOCATION,
		PS_FREE,
		PS_USED,
		PS_FILENLEN,

		PS_FINISH,
		PS_ERROR,
	} state;               // State based on found XML tags
	int depth;             // XML tag depth
	char *mem_location;
	unsigned long mem_used;
	unsigned long mem_free;
	unsigned long mem_namelen;

	int path_match_len;    // Needed to find best-matching entry

	/* Output */
	const char *error;
	struct statfs meminfo;
};

static void xml_element_start(void* user_data, const char* name, const char** UNUSED(attrs))
{
	struct parsing_data *data = (struct parsing_data*)user_data;

	switch (data->state) {
	case PS_ROOT:
		if (data->depth == 0 && strcmp(name, "Capability") == 0) {
			data->state = PS_CAPABILITY;
		} else {
			data->error = "Root element must be \"Capability\"";
			data->state = PS_ERROR;
		}
		break;

	case PS_CAPABILITY:
		if (data->depth == 1 && strcmp(name, "General") == 0)
			data->state = PS_GENERAL;
		break;

	case PS_GENERAL:
		if (data->depth == 2 && strcmp(name, "Memory") == 0)
			data->state = PS_MEMORY;
		break;

	case PS_MEMORY:
		if (data->depth == 3) {
			if (strcmp(name, "MemType") == 0)
				data->state = PS_MEM_TYPE;
			else if (strcmp(name, "Location") == 0)
				data->state = PS_LOCATION;
			else if (strcmp(name, "Free") == 0)
				data->state = PS_FREE;
			else if (strcmp(name, "Used") == 0)
				data->state = PS_USED;
			else if (strcmp(name, "FileNLen") == 0)
				data->state = PS_FILENLEN;
		}
		break;

	case PS_MEM_TYPE:
	case PS_LOCATION:
	case PS_FREE:
	case PS_USED:
	case PS_FILENLEN:
		// Memory properties must be plain text.
		data->error = "Memory properties must be plain text, no elements inside are allowed";
		data->state = PS_ERROR;
		break;

	case PS_FINISH:
	case PS_ERROR:
		break;
	}

	++(data->depth);
}

static void xml_element_end(void* user_data, const char* name)
{
	struct parsing_data *data = (struct parsing_data*)user_data;
	(data->depth)--;

	switch (data->state) {
	case PS_ROOT:
		break;

	case PS_CAPABILITY:
		if (data->depth == 0 && strcmp(name, "Capability") == 0) {
			data->error = "\"General\" section not found";
			data->state = PS_ERROR;
		}
		break;

	case PS_GENERAL:
		if (data->depth == 1 && strcmp(name, "General") == 0)
			data->state = PS_FINISH;
		break;

	case PS_MEMORY:
		if (data->depth == 2 && strcmp(name, "Memory") == 0)
		{

			bool use_memory_entry = false;
			ssize_t len = data->mem_location? strlen(data->mem_location): 0;

			data->state = PS_GENERAL;

			if (len == 0 && data->path_match_len < 0)
			{
				/* use memory entry in case that it contains no/empty
				 * location entry and there was no previous location
				 */
				use_memory_entry = true;
				data->path_match_len = 0;
			}
			else
			{
				/* Check if this memory entity is a better match */
				if (data->path_match_len < len &&
				    len <= (int)strlen(data->path) &&
				    strncasecmp(data->path, data->mem_location, len) == 0 &&
				    (data->path[len] == '/' || data->path[len] == 0))
				{
					use_memory_entry = true;
					data->path_match_len = len;
				}
			}

			if (use_memory_entry)
			{
				data->meminfo.f_bsize = 1;
				data->meminfo.f_blocks = data->mem_used + data->mem_free;
				data->meminfo.f_bfree = data->mem_free;
				data->meminfo.f_bavail = data->mem_free;
				data->meminfo.f_namelen = data->mem_namelen;
			}

			if (data->mem_location)
				free(data->mem_location);
		}
		break;

	case PS_MEM_TYPE:
	case PS_LOCATION:
	case PS_FREE:
	case PS_USED:
	case PS_FILENLEN:
		data->state = PS_MEMORY;
		break;

	case PS_FINISH:
	case PS_ERROR:
		break;
	}
}

static unsigned long parse_ulong(const char* str) {
	long result;
	errno = 0;
	result = strtol(str, NULL, 10);
	//check invalid string or invalid value
	if (errno == 0 || result >= 0)
		return (unsigned long)result;
	return 0UL;
}

static void xml_character_data(void* user_data, const char* str, int len)
{
	struct parsing_data *data = (struct parsing_data*)user_data;
	int i;
	char *tmp = NULL;

	switch (data->state) {
	case PS_MEM_TYPE:
		break;

	case PS_LOCATION:
		if (len > 0)
		{
			data->mem_location = strndup(str, len);
			/* Convert path separators to unix type */
			for (i = 0; i < len; ++i)
			{
				if (data->mem_location[i] == '\\')
					data->mem_location[i] = '/';
			}
			/* Remove trailing directory separator */
			if (data->mem_location[len-1] == '/')
			  data->mem_location[len-1] = 0;
		}
		break;

	case PS_FREE:
		tmp = strndup(str, len);
		data->mem_free = parse_ulong(tmp);
		break;

	case PS_USED:
		tmp = strndup(str, len);
		data->mem_used = parse_ulong(tmp);
		break;

	case PS_FILENLEN:
		tmp = strndup(str, len);
		data->mem_namelen = parse_ulong(tmp);
		break;

	default:
		break;
	}

	if (tmp)
		free(tmp);
}

static int ofs_statfs(const char *path, struct statfs *st)
{
	int res;
	char *tpath = translate_path(path);

	DEBUG("%s() %s\n", __FUNCTION__, path);

	memset(st, 0, sizeof(*st));
	st->f_bsize = 1;
	st->f_blocks = report_space;
	st->f_bfree = report_space;
	st->f_bavail = report_space;

	res = ofs_connect();
	if(res < 0)
		return res; /* errno */

#ifdef SIEMENS_S45
	/* Retrieve Infos */
	(void) obexftp_info(cli, 0x01);
	st->f_blocks = cli->apparam_info;
	(void) obexftp_info(cli, 0x02);
	st->f_bfree = cli->apparam_info;
	st->f_bavail = st->f_bfree;

#else
	(void) obexftp_setpath(cli, tpath, 1);
	res = obexftp_get_type(cli, XOBEX_CAPABILITY, 0, 0);
	if (res >= 0) {
		XML_Parser parser;

		struct parsing_data data;
		memset(&data, 0, sizeof(data));
		data.path = tpath + 1;
		data.path_match_len = -1;

		parser = XML_ParserCreate(NULL);
		XML_SetUserData(parser, &data);
		XML_SetElementHandler(parser, xml_element_start, xml_element_end);
		XML_SetCharacterDataHandler(parser, xml_character_data);
		if (XML_Parse(parser, cli->buf_data, cli->buf_size, 1)) {
			if (data.error) {
				DEBUG("%s(): PARSE ERROR: %s\n", __func__, data.error);
			} else if (data.path_match_len >= 0) {
				*st = data.meminfo;
			}
		} else {
			DEBUG("%s(): PARSE ERROR: %s at line %d\n", __func__,
				XML_ErrorString(XML_GetErrorCode(parser)),
				XML_GetCurrentLineNumber(parser)
			);
		}
		XML_ParserFree(parser);
	}
#endif
	DEBUG("%s() GOT FS STAT: %" PRId64 " / %" PRId64 "\n", __func__, st->f_bfree, st->f_blocks);
	ofs_disconnect();

	free(tpath);

	return 0;
}

static void *ofs_init(void) {
	//cli_open();
	return NULL;
}

static void ofs_destroy(void *UNUSED(private_data)) {
	fprintf(stderr, "terminating...\n");
	cli_close();
	return;
}

static int ofs_utime(const char *a, struct utimbuf *b)
{
	return 0;
}

static int ofs_chmod(const char *a, mode_t b)
{
	return 0;
}

static int ofs_chown(const char *a, uid_t b, gid_t c)
{
	return 0;
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
	chmod:		ofs_chmod,
	chown:		ofs_chown,
	truncate:	ofs_truncate,
	utime:		ofs_utime,
	open:		ofs_open,
	read:		ofs_read,
	write:		ofs_write,
	statfs:		ofs_statfs,
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
			{"root",	required_argument, NULL, 'r'},
			{"nonblock",	no_argument, NULL, 'N'},
			{"report-space",required_argument, NULL, 'S'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'h'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "+ib:B:d:u:t:n:r:NS:h",
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
				char *p  =strchr(optarg, ':');
				if (p) {
					*p = '\0';
					channel = atoi(++p);
				}
			}
			{
                                int n;
                                if (sscanf(optarg, "%d.%d.%d.%d", &n, &n, &n, &n) != 4)
                                        fprintf(stderr, "Please use dotted quad notation.\n");
                        }
			break;
			
		case 'r':
			root = optarg;
			break;

		case 'N':
			nonblock = true;
			break;

		case 'S':
			report_space = atoi(optarg);
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
				" -r, --root <path>           path on device to use as root\n\n"
				" -N, --nonblock              nonblocking mode\n"
				" -S, --report-space <bytes>  report this number as total/free space\n\n"
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
	if (root == NULL) root = "";
	root_len = strlen(root);
	if (root[root_len-1] == '/' || root[root_len-1] == '\\') {
		root[root_len-1] = '\0';
		--root_len;
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
