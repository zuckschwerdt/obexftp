/**
	\file obexftp/client.h
	ObexFTP client API.
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

#ifndef OBEXFTP_CLIENT_H
#define OBEXFTP_CLIENT_H

#include <inttypes.h>
#include <sys/stat.h>
#include <time.h>
#include <openobex/obex.h>
#ifndef OBEX_TRANS_USB
#define OBEX_TRANS_USB		6
#endif
#include "obexftp.h"
#include "object.h"
#include "uuid.h"

#ifdef __cplusplus
extern "C" {
#endif


/* quirks */

#define OBEXFTP_LEADING_SLASH	0x01	/* used in get (and alike) */
#define OBEXFTP_TRAILING_SLASH	0x02	/* used in list */
#define OBEXFTP_SPLIT_SETPATH	0x04	/* some phones dont have a cwd */
#define OBEXFTP_CONN_HEADER	0x08	/* do we even need this? */

#define OBEXFTP_USE_LEADING_SLASH(x)	((x & OBEXFTP_LEADING_SLASH) != 0)
#define OBEXFTP_USE_TRAILING_SLASH(x)	((x & OBEXFTP_TRAILING_SLASH) != 0)
#define OBEXFTP_USE_SPLIT_SETPATH(x)	((x & OBEXFTP_SPLIT_SETPATH) != 0)
#define OBEXFTP_USE_CONN_HEADER(x)	((x & OBEXFTP_CONN_HEADER) != 0)

/* dont disable leading slashes unless you disable split setpath */
#define DEFAULT_OBEXFTP_QUIRKS	\
	(OBEXFTP_LEADING_SLASH | OBEXFTP_TRAILING_SLASH | OBEXFTP_SPLIT_SETPATH | OBEXFTP_CONN_HEADER)
#define DEFAULT_CACHE_TIMEOUT 180	/* 3 minutes */
#define DEFAULT_CACHE_MAXSIZE 10240	/* 10k */

/* types */

typedef struct {
	char name[256];
	mode_t mode;
	int size;
	time_t mtime;
} stat_entry_t;

typedef struct cache_object cache_object_t;
struct cache_object
{
	cache_object_t *next;
	int refcnt;
	time_t timestamp;
	int size;	/* or uint32_t */
	char *name;
	char *content;	/* or uint8_t */
	stat_entry_t *stats;	/* only if its a parsed directory */
};

typedef struct {
	/* state */
	obex_t *obexhandle;
	uint32_t connection_id; /* set to 0xffffffff if unused */
	obex_ctrans_t *ctrans; /* only valid with OBEX_TRANS_CUSTOM */
	int transport; /* the transport for obexhandle */
	int finished;
	int success;
	int obex_rsp;
	int mutex;	/* should be using pthreads for this */
	int quirks;
	/* client */
	obexftp_info_cb_t infocb;
	void *infocb_data;
	/* transfer (put) */
	int fd; /* used in put body */
	uint8_t *stream_chunk;
	uint32_t out_size;
	uint32_t out_pos;
	const uint8_t *out_data;
	/* transfer (get) */
	char *target_fn; /* used in get body */
	uint32_t buf_size; /* not size but len... */
	uint8_t *buf_data;
	uint32_t apparam_info;
	/* persistence */
	cache_object_t *cache;
	int cache_timeout;
	int cache_maxsize;
} obexftp_client_t;


/* session */

/*@null@*/ obexftp_client_t *obexftp_open(int transport,
				 /*@null@*/ /*const*/ obex_ctrans_t *ctrans,
				 /*@null@*/ obexftp_info_cb_t infocb,
				 /*@null@*/ void *infocb_data);

void obexftp_close(/*@only@*/ /*@out@*/ /*@null@*/ obexftp_client_t *cli);

int obexftp_connect_uuid(obexftp_client_t *cli,
				/*@null@*/ const char *device, /* for INET, BLUETOOTH */
				int port, /* INET(?), BLUETOOTH, USB*/
				/*@null@*/ const uint8_t uuid[], uint32_t uuid_len);
int obexftp_connect_src(obexftp_client_t *cli,
				/*@null@*/ const char *src, /* HCI no. or address */
				/*@null@*/ const char *device, /* for INET, BLUETOOTH */
				int port, /* INET(?), BLUETOOTH, USB*/
				/*@null@*/ const uint8_t uuid[], uint32_t uuid_len);
int obexftp_connect_service(obexftp_client_t *cli,
				/*@null@*/ const char *src, /* HCI no. or address */
				/*@null@*/ const char *device, /* for INET, BLUETOOTH */
				int port, /* INET(?), BLUETOOTH, USB*/
				int service);

//int obexftp_connect_service(obexftp_client_t *cli,
//				/*@null@*/ const char *service,
//				/*@null@*/ const char *device, /* for INET, BLUETOOTH */
//				int port, /* INET(?), BLUETOOTH, USB*/ );

#define	obexftp_connect(cli, device, port) \
	obexftp_connect_uuid(cli, device, port, UUID_FBS, sizeof(UUID_FBS))
#define	obexftp_connect_ftp(cli, device, port) \
	obexftp_connect_uuid(cli, device, port, UUID_FBS, sizeof(UUID_FBS))
#define	obexftp_connect_push(cli, device, port) \
	obexftp_connect_uuid(cli, device, port, NULL, 0)
#define	obexftp_connect_sync(cli, device, port) \
	obexftp_connect_uuid(cli, device, port, UUID_IRMC, sizeof(UUID_IRMC))

int obexftp_disconnect(obexftp_client_t *cli);


/* transfer */

int obexftp_setpath(obexftp_client_t *cli, /*@null@*/ const char *name, int create);

#define	obexftp_chpath(cli, name) \
	obexftp_setpath(cli, name, 0)

#define	obexftp_mkpath(cli, name) \
	obexftp_setpath(cli, name, 1)

#define	obexftp_cdup(cli) \
	obexftp_setpath(cli, NULL, 0)

#define	obexftp_cdtop(cli) \
	obexftp_setpath(cli, "", 0)

int obexftp_get_type(obexftp_client_t *cli,
		 const char *type,
		 /*@null@*/ const char *localname,
		 /*@null@*/ const char *remotename);

#define	obexftp_get(cli, localname, remotename) \
	obexftp_get_type(cli, NULL, localname, remotename)

#define	obexftp_list(cli, localname, remotename) \
	obexftp_get_type(cli, XOBEX_LISTING, localname, remotename)

#define	obexftp_get_capability(cli, localname, remotename) \
	obexftp_get_type(cli, XOBEX_CAPABILITY, localname, remotename)

int obexftp_put_file(obexftp_client_t *cli, const char *filename,
		     const char *remotename);

int obexftp_put_data(obexftp_client_t *cli, const char *data, int size,
		     const char *remotename);

int obexftp_del(obexftp_client_t *cli, const char *name);


/* Siemens only */

int obexftp_info(obexftp_client_t *cli, uint8_t opcode);

int obexftp_rename(obexftp_client_t *cli,
		   const char *sourcename,
		   const char *targetname);


/* compatible directory handling */

void *obexftp_opendir(obexftp_client_t *cli, const char *name);

int obexftp_closedir(void *dir);

stat_entry_t *obexftp_readdir(void *dir);

stat_entry_t *obexftp_stat(obexftp_client_t *cli, const char *name);


#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_CLIENT_H */

