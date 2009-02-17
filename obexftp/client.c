/**
	\file obexftp/client.c
	ObexFTP client API implementation.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>

#ifdef _WIN32
#define O_BINARY (_O_BINARY)
#define CREATE_MODE_FILE (S_IRUSR|S_IWUSR)
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define O_BINARY (0)
#define CREATE_MODE_FILE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#endif /* _WIN32 */

#ifdef HAVE_BLUETOOTH
#include "bt_kit.h"
#else
#define ESOCKTNOSUPPORT	WSAESOCKTNOSUPPORT
#endif /* HAVE_BLUETOOTH */

#include <openobex/obex.h>
#include <multicobex/multi_cobex.h>

#include "obexftp.h"
#include "client.h"
#include "object.h"
#include "obexftp_io.h"
#include "uuid.h"
#include "cache.h"

#include <common.h>


#pragma pack(1)
typedef struct { /* fixed to 6 bytes for now */
	uint8_t code;
	uint8_t info_len;
	uint8_t info[4];
} apparam_t;
#pragma pack()


/**
	Empty callback used as default.
 */
static void dummy_info_cb(int UNUSED(event), const char *UNUSED(msg), int UNUSED(len), void *UNUSED(data))
{
	/* do nothing */
}


/**
	Normalize the path argument.
	\note
	wont turn relative paths into (most likely wrong) absolute ones.
	wont expand "../" or "./".
 */
/*
static char *normalize_file_path(const char *name)
{
	char *p, *copy;

	return_val_if_fail(name != NULL, NULL);

	p = copy = malloc(strlen(name) + 1); / * cant be longer, can it? * /
/ *
	if (OBEXFTP_USE_LEADING_SLASH(quirks))
		*p++ = '/';
	while (*name == '/') name++;
* /
	while (*name) {
		if (*name == '/') {
			*p++ = *name++;
			while (*name == '/') name++;
		} else {
			*p++ = *name++;
		}
	}

	*p = '\0';

	return copy;
}
*/


/**
	Normalize the path argument and split into pathname and basename.
	\note
	Wont turn relative paths into (most likely wrong) absolute ones.
	Wont expand "../" or "./".
	Will keep "telecom" prefix.
	\warning
	Do not use this function if there is no slash in the argument!
 */
static void split_file_path(const char *name, /*@only@*/ char **basepath, /*@only@*/ char **basename)
{
	char *p;
	const char *tail;

	return_if_fail(name != NULL);

        for (tail = name; *tail == '/'; tail++);
	if (!strncmp(tail, "telecom/", 8)) {
		*basename = strdup(tail); /* keep whole path */
		*basepath = strdup(""); /* cd top */
		return;
	}
	
	tail = strrchr(name, '/');
	if (tail)
		tail++;
	else
		tail = name;

	if (basename)
		*basename = strdup(tail);

	if (!basepath)
		return;

	p = *basepath = malloc(strlen(name) + 1); /* cant be longer, can it? */
/*
	if (OBEXFTP_USE_LEADING_SLASH(quirks))
		*p++ = '/';
	while (*name == '/') name++;
*/
	while (*name && name < tail) {
		if (*name == '/') {
			*p++ = *name++;
			while (*name == '/') name++;
		} else {
			*p++ = *name++;
		}
	}
/*
	if (p > *basepath && *(p-1) == '/')
		p--;
	if (OBEXFTP_USE_TRAILING_SLASH(quirks))
		*p++ = '/';
*/
	*p = '\0';
}


/**
	Add more data from memory to stream.
 */
static int cli_fillstream_from_memory(obexftp_client_t *cli, obex_object_t *object)
{
	obex_headerdata_t hv;
	int actual = cli->out_size - cli->out_pos;
	if (actual > STREAM_CHUNK)
		actual = STREAM_CHUNK;
	DEBUG(3, "%s() Read %d bytes\n", __func__, actual);
	
	if(actual > 0) {
		/* Read was ok! */
		hv.bs = (const uint8_t *) &cli->out_data[cli->out_pos];
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hv, actual, OBEX_FL_STREAM_DATA);
		cli->out_pos += actual;
	}
	else if(actual == 0) {
		/* EOF */
		cli->out_data = NULL; /* dont free, isnt ours */
		hv.bs = (const uint8_t *) &cli->out_data[cli->out_pos];
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hv, 0, OBEX_FL_STREAM_DATAEND);
	}
	else {
		/* Error */
		cli->out_data = NULL; /* dont free, isnt ours */
		hv.bs = (const uint8_t *) NULL;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hv, 0, OBEX_FL_STREAM_DATA);
	}

	return actual;
}


/**
	Add more data from file to stream.
 */
static int cli_fillstream_from_file(obexftp_client_t *cli, obex_object_t *object)
{
	obex_headerdata_t hv;
	int actual;
		
	DEBUG(3, "%s()\n", __func__);
	
	actual = read(cli->fd, cli->stream_chunk, STREAM_CHUNK);
	
	DEBUG(3, "%s() Read %d bytes\n", __func__, actual);
	
	if(actual > 0) {
		/* Read was ok! */
		hv.bs = (const uint8_t *) cli->stream_chunk;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hv, actual, OBEX_FL_STREAM_DATA);
	}
	else if(actual == 0) {
		/* EOF */
		(void) close(cli->fd);
		cli->fd = -1;
		hv.bs = (const uint8_t *) cli->stream_chunk;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hv, 0, OBEX_FL_STREAM_DATAEND);
	}
        else {
		/* Error */
		(void) close(cli->fd);
		cli->fd = -1;
		hv.bs = (const uint8_t *) NULL;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hv, 0, OBEX_FL_STREAM_DATA);
	}

	return actual;
}


/**
	Save body from object or return application parameters.
 */
static void client_done(obex_t *handle, obex_object_t *object, int UNUSED(obex_cmd), int UNUSED(obex_rsp))
{
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;
	const apparam_t *app = NULL;
	uint8_t *p;

        const uint8_t *body_data = NULL;
	uint32_t body_len = -1;

	/*@temp@*/ obexftp_client_t *cli;

	cli = OBEX_GetUserData(handle);

	DEBUG(3, "%s()\n", __func__);

	if (cli->fd > 0)
		(void) close(cli->fd);
	if (cli->buf_data) {
		DEBUG(1, "%s: Warning: buffer still active?\n", __func__);
	}

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen)) {
		if(hi == OBEX_HDR_BODY) {
			DEBUG(3, "%s() Found body (length: %d)\n", __func__, hlen);
			if (cli->target_fn == NULL) {
				if (cli->buf_data) {
					DEBUG(1, "%s: Warning: purging non-empty buffer.\n", __func__);
					/* ok to free since we must have malloc' it right here */
					free(cli->buf_data);
				}
				p = malloc(hlen + 1);
				if (p) {
					memcpy(p, hv.bs, hlen);
					p[hlen] = '\0';
					cli->buf_size = hlen;
					cli->buf_data = p;
				}
			}
			body_len = hlen;
			body_data = hv.bs;
			cli->infocb(OBEXFTP_EV_BODY, hv.bs, hlen, cli->infocb_data);
			DEBUG(3, "%s() Done body\n", __func__);
                        /* break; */
                }
                else if(hi == OBEX_HDR_CONNECTION) {
			DEBUG(3, "%s() Found connection number: %d\n", __func__, hv.bq4);
			cli->connection_id = hv.bq4;
		}
                else if(hi == OBEX_HDR_WHO) {
			DEBUG(3, "%s() Sender identified\n", __func__);
		}
                else if(hi == OBEX_HDR_NAME) {
			DEBUG(3, "%s() Sender name\n", __func__);
			DEBUGBUFFER(hv.bs, hlen);
		}
                else if(hi == OBEX_HDR_APPARAM) {
			DEBUG(3, "%s() Found application parameters\n", __func__);
                        if(hlen == sizeof(apparam_t)) {
				app = (const apparam_t *)hv.bs;
				/* order is network byte order (big-endian) */
				cli->apparam_info = (app->info[0] << (3*8)) + (app->info[1] << (2*8)) +
				                    (app->info[2] << (1*8)) + (app->info[3] << (0*8));
				cli->infocb(OBEXFTP_EV_INFO, (char*)&cli->apparam_info, 0, cli->infocb_data);
			}
			else
				DEBUG(3, "%s() Application parameters don't fit %d vs. %d.\n", __func__, hlen, sizeof(apparam_t));
                        break;
                }
                else    {
                        DEBUG(3, "%s() Skipped header %02x\n", __func__, hi);
                }
        }

        if(body_data) {
		if (body_len > 0) {
			if (cli->target_fn != NULL) {
				/* simple body writer */
				int fd;
				//fd = open_safe("", cli-> target_fn);
				fd = open(cli-> target_fn, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, CREATE_MODE_FILE);
				if(fd > 0) {
					(void) write(fd, body_data, body_len);
					(void) close(fd);
				} else {
					DEBUG(3, "%s() Error writing body\n", __func__);
				}
				free (cli->target_fn);
				cli->target_fn = NULL;
			} else {
				DEBUG(3, "%s() Body not written\n", __func__);
			}
		} else {
			DEBUG(3, "%s() Skipping empty body\n", __func__);
		}
        }
        if(app) {
		DEBUG(3, "%s() Appcode %d, data (%d) %d\n", __func__,
			app->code, app->info_len, cli->apparam_info);

        }
}


/**
	Handle incoming event from OpenOBEX.
 */
static void cli_obex_event(obex_t *handle, obex_object_t *object, int UNUSED(mode), int event, int obex_cmd, int obex_rsp)
{
	/*@temp@*/ obexftp_client_t *cli;

	cli = OBEX_GetUserData(handle);

	switch (event)	{
	case OBEX_EV_PROGRESS:
		cli->infocb(OBEXFTP_EV_PROGRESS, "", 0, cli->infocb_data);
		break;
	case OBEX_EV_REQDONE:
		cli->finished = TRUE;
		if(obex_rsp == OBEX_RSP_SUCCESS)
			cli->success = TRUE;
		else {
			cli->success = FALSE;
			DEBUG(2, "%s() OBEX_EV_REQDONE: obex_rsp=%02x\n", __func__, obex_rsp);
		}
		cli->obex_rsp = obex_rsp;
		client_done(handle, object, obex_cmd, obex_rsp);
		break;
	
	case OBEX_EV_LINKERR:
		cli->finished = TRUE;
		cli->success = FALSE;
		DEBUG(2, "%s() OBEX_EV_LINKERR\n", __func__);
		break;
	
	case OBEX_EV_STREAMEMPTY:
		if (cli->out_data)
			(void) cli_fillstream_from_memory(cli, object);
		else
			(void) cli_fillstream_from_file(cli, object);
		break;
	
	default:
		DEBUG(1, "%s() Unknown event %d\n", __func__, event);
		break;
	}
}


/**
	Wait for the OBEX client to finish.
 */
static int obexftp_sync(obexftp_client_t *cli)
{
	int ret;
	DEBUG(3, "%s()\n", __func__);

	/* cli->finished = FALSE; */

	while(cli->finished == FALSE) {
		ret = OBEX_HandleInput(cli->obexhandle, 20);
		DEBUG(3, "%s() OBEX_HandleInput = %d\n", __func__, ret);

		if (ret <= 0) {
			DEBUG(2, "%s() OBEX_HandleInput error: %d\n", __func__, errno);
			return -1;
		}
	}

	DEBUG(3, "%s() Done success=%d\n", __func__, cli->success);

	if(cli->success)
		return 1;
	else
		return - cli->obex_rsp;
}


/**
	Do an OBEX request synchronous.
 */
static int cli_sync_request(obexftp_client_t *cli, obex_object_t *object)
{
	DEBUG(3, "%s()\n", __func__);

	if (cli->finished == FALSE)
		return -EBUSY;
	cli->finished = FALSE;
	(void) OBEX_Request(cli->obexhandle, object);

	return obexftp_sync (cli);
}


/**
	Create an obexftp client.

	\param transport the transport type that will be used
	\param ctrans optional custom transport (don't use)
	\param infocb optional info callback
	\param infocb_data optional info callback data

	\return a new allocated ObexFTP client instance, NULL on error
 */
obexftp_client_t *obexftp_open(int transport, /*const*/ obex_ctrans_t *ctrans, obexftp_info_cb_t infocb, void *infocb_data)
{
	obexftp_client_t *cli;

	DEBUG(3, "%s()\n", __func__);
	cli = calloc (1, sizeof(obexftp_client_t));
	if(cli == NULL)
		return NULL;

	cli->finished = TRUE;

	if (infocb)
		cli->infocb = infocb;
	else
		cli->infocb = dummy_info_cb;
	cli->infocb_data = infocb_data;
		
	cli->quirks = DEFAULT_OBEXFTP_QUIRKS;
	cli->cache_timeout = DEFAULT_CACHE_TIMEOUT;
	cli->cache_maxsize = DEFAULT_CACHE_MAXSIZE;

	cli->fd = -1;

       	cli->obexhandle = OBEX_Init(transport, cli_obex_event, 0);

	if(cli->obexhandle == NULL) {
		free(cli);
		return NULL;
	}
	cli->transport = transport;

	if ( ctrans ) {
                DEBUG(2, "Custom  OBEX transport requested!\n");
		/* OBEX_RegisterCTransport is const to ctrans ... */
                if(OBEX_RegisterCTransport(cli->obexhandle, ctrans) < 0) {
                        DEBUG(1, "Custom transport callback-registration failed\n");
                }
		cli->ctrans = ctrans;
        }

	OBEX_SetUserData(cli->obexhandle, cli);
	
	/* Buffer for body */
	cli->stream_chunk = malloc(STREAM_CHUNK);
	if(cli->stream_chunk == NULL) {
		free(cli);
		return NULL;
	}
	return cli;
}


/**
	Close an obexftp client and free the resources.

	\param cli
		the obexftp_client_t to be shut done and free'd.
		It's save to pass NULL here.

	Closes the given obexftp client and frees the resources.
	It's recommended to set the client reference to NULL afterwards.
 */
void obexftp_close(obexftp_client_t *cli)
{
	DEBUG(3, "%s()\n", __func__);
	return_if_fail(cli != NULL);

	OBEX_Cleanup(cli->obexhandle);
	if (cli->buf_data) {
		DEBUG(1, "%s: Warning: purging left-over buffer.\n", __func__);
		free(cli->buf_data);
	}
	cache_purge(&cli->cache, NULL);
	free(cli->stream_chunk);
	free(cli);
}


/**
	Do simple connect as client.

	\param cli an obexftp_client_t created by obexftp_open().
	\param device the device address to connect to (transport specific)
	\param port the port/channel for the device address
	\param uuid UUID string for CONNECT (no default)
	\param uuid_len length of the UUID string (excluding terminating zero)

	\return the result of the CONNECT request, -1 on error

	\note Wrapper function for obexftp_connect_src()
	\warning Always use a UUID (except for OBEX PUSH)
 */
int obexftp_connect_uuid(obexftp_client_t *cli, const char *device, int port, const uint8_t uuid[], uint32_t uuid_len)
{
	return obexftp_connect_src(cli, NULL, device, port, uuid, uuid_len);
}


int obexftp_connect_service(obexftp_client_t *cli, const char *src, const char *device, int port, int service)
{
	uint8_t *uuid = NULL;
	uint32_t uuid_len = 0;
	if (service == OBEX_FTP_SERVICE) {
		uuid = UUID_FBS;
		uuid_len = sizeof(UUID_FBS);
	}
	if (service == OBEX_SYNC_SERVICE) {
		uuid = UUID_IRMC;
		uuid_len = sizeof(UUID_IRMC);
	}
	// otherwiese default to OBEX_PUSH_SERVICE
	return obexftp_connect_src(cli, src, device, port, uuid, uuid_len);
}



/**
	Connect this ObexFTP client using a given source address by sending an OBEX CONNECT request.

	\param cli an obexftp_client_t created by obexftp_open().
	\param src optional local source interface address (transport specific)
	\param device the device address to connect to (transport specific)
	\param port the port/channel for the device address
	\param uuid UUID string for CONNECT (no default)
	\param uuid_len length of the UUID string (excluding terminating zero)

	\return the result of the CONNECT request, -1 on error

	\note Always use a UUID (except for OBEX PUSH)
 */
int obexftp_connect_src(obexftp_client_t *cli, const char *src, const char *device, int port, const uint8_t uuid[], uint32_t uuid_len)
{
	struct sockaddr_in peer;
#ifdef HAVE_BLUETOOTH
	char *devicedup, *devicep;
	bdaddr_t bdaddr, src_addr;
#endif
#ifdef HAVE_USB
	int obex_intf_cnt;
	obex_interface_t *obex_intf;
#endif
	obex_object_t *object;
	obex_headerdata_t hv;
	int ret = -1; /* no connection yet */

	DEBUG(3, "%s()\n", __func__);
	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_CONNECTING, "", 0, cli->infocb_data);

	switch (cli->transport) {

	case OBEX_TRANS_IRDA:
		ret = IrOBEX_TransportConnect(cli->obexhandle, "OBEX");
		DEBUG(3, "%s() IR %d\n", __func__, ret);
		break;
		
	case OBEX_TRANS_INET:
		if (!device) {
			ret = -EINVAL;
			break;
		}
#ifdef _WIN32
		peer.sin_addr.s_addr = inet_addr(device);
		ret = (peer.sin_addr.s_addr == INADDR_NONE) ? 0 : 1;
#else
		ret = inet_aton(device, &peer.sin_addr);
#endif
		if (ret) {
			peer.sin_family = AF_INET;
			peer.sin_port = htons(port); /* overridden with OBEX_PORT 650 anyhow */
			ret = InOBEX_TransportConnect(cli->obexhandle, (struct sockaddr *) &peer,
						  sizeof(struct sockaddr_in));
			DEBUG(3, "%s() TCP %d\n", __func__, ret);
		} else
			ret = -EINVAL; /* is there a better errno? */
		break;

	case OBEX_TRANS_CUSTOM:
		/* don't change the custom transport once it is in place */
		if (cli->ctrans == NULL) {
			cli->ctrans = cobex_ctrans (device);
                	if(OBEX_RegisterCTransport(cli->obexhandle, cli->ctrans) < 0) {
                        	DEBUG(1, "Custom transport callback-registration failed\n");
	                }
	        }
		ret = OBEX_TransportConnect(cli->obexhandle, NULL, 0);
		DEBUG(3, "%s() TC %d\n", __func__, ret);
		break;

#ifdef HAVE_BLUETOOTH
	case OBEX_TRANS_BLUETOOTH:
		if (!src) {
			bacpy(&src_addr, BDADDR_ANY);
		}
#ifdef HAVE_SDPLIB
		else if (!strncmp(src, "hci", 3)) {
			hci_devba(atoi(src + 3), &src_addr);
		}
		else if (atoi(src) != 0) {
			hci_devba(atoi(src), &src_addr);
		}
#endif
		else {
			str2ba(src, &src_addr);
		}
		if (!device) {
			ret = -EINVAL;
			break;
		}
		if (port < 1) {
			port = obexftp_browse_bt(device, 0);
		}
		/* transform some chars to colons */
		devicedup = devicep = strdup(device);
		for (; *devicep; devicep++) {
			if (*devicep == '-') *devicep = ':';
			if (*devicep == '_') *devicep = ':';
			if (*devicep == '/') *devicep = ':';
		}
		(void) str2ba(devicedup, &bdaddr);
		free(devicedup);
		ret = BtOBEX_TransportConnect(cli->obexhandle, &src_addr, &bdaddr, (uint8_t)port);
		DEBUG(3, "%s() BT %d\n", __func__, ret);
		break;
#endif /* HAVE_BLUETOOTH */

#ifdef HAVE_USB
	case OBEX_TRANS_USB:
		obex_intf_cnt = OBEX_FindInterfaces(cli->obexhandle, &obex_intf);
		DEBUG(3, "%s() \n", __func__);
		if (obex_intf_cnt <= 0) {
			DEBUG(1, "%s() there are no valid USB interfaces\n", __func__);
			ret = -EINVAL; /* is there a better errno? */
		}
		else if (port >= obex_intf_cnt) {
			DEBUG(1, "%s() %d is an invalid USB interface number\n", __func__, port);
			ret = -EINVAL; /* is there a better errno? */
		} else
			ret = OBEX_InterfaceConnect(cli->obexhandle, &obex_intf[port]);
		DEBUG(3, "%s() USB %d\n", __func__, ret);
		break;
#endif /* HAVE_USB */

	default:
		ret = -ESOCKTNOSUPPORT;
		break;
	}

	if (ret < 0) {
		/* could be -EBUSY or -ESOCKTNOSUPPORT */
		cli->infocb(OBEXFTP_EV_ERR, "connect", 0, cli->infocb_data);
		return ret;
	}

#ifdef COMPAT_S45
	// try S45 UUID first.
	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_CONNECT);
	hv.bs = UUID_S45;
	if(OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_TARGET,
                                hv, sizeof(UUID_S45), OBEX_FL_FIT_ONE_PACKET) < 0) {
                DEBUG(1, "Error adding header\n");
                OBEX_ObjectDelete(cli->obexhandle, object);
                return -1;
        }
	cli->connection_id = 0xffffffff;
	ret = cli_sync_request(cli, object);

	if(ret < 0) {
		cli->infocb(OBEXFTP_EV_ERR, "S45 UUID", 0, cli->infocb_data);
#endif
		object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_CONNECT);
		if (uuid) {
			hv.bs = uuid;
			if(OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_TARGET,
        	        	                hv, uuid_len, OBEX_FL_FIT_ONE_PACKET) < 0) {
	        	        DEBUG(1, "Error adding header\n");
		                OBEX_ObjectDelete(cli->obexhandle, object);
                		return -1;
	        	}
		}
		cli->connection_id = 0xffffffff;
		ret = cli_sync_request(cli, object);
		if (!OBEXFTP_USE_CONN_HEADER(cli->quirks))
			cli->connection_id = 0xffffffff;
#ifdef COMPAT_S45
	}
#endif

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, "send UUID", 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, "", 0, cli->infocb_data);

	return ret;
}


/**
	Disconnect this ObexFTP client by sending an OBEX DISCONNECT request.

	\param cli an obexftp_client_t created by obexftp_open().

	\return the result of the DISCONNECT request
 */
int obexftp_disconnect(obexftp_client_t *cli)
{
	obex_object_t *object;
	obex_headerdata_t hv;
	int ret;

	DEBUG(3, "%s()\n", __func__);
	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_DISCONNECTING, "", 0, cli->infocb_data);

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_DISCONNECT);
	if(cli->connection_id != 0xffffffff) {
		hv.bq4 = cli->connection_id;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_CONNECTION,
				    hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}
	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, "disconnect", 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, "", 0, cli->infocb_data);

	/* don't -- obexftp_close will handle this with OBEX_Cleanup */
	/* OBEX_TransportDisconnect(cli->obexhandle); */
	return ret;
}


/**
	Send a custom Siemens OBEX app info opcode.

	\param cli an obexftp_client_t created by obexftp_open().
	\param opcode the info opcode,
		0x01 to inquire installed memory, 0x02 to get free memory

	\return the result of the app info request
 */
int obexftp_info(obexftp_client_t *cli, uint8_t opcode)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_RECEIVING, "info", 0, cli->infocb_data);

	DEBUG(2, "%s() Retrieving info %d\n", __func__, opcode);

        object = obexftp_build_info (cli->obexhandle, cli->connection_id, opcode);
        if(object == NULL)
                return -1;
 
	ret = cli_sync_request(cli, object);
		
	if(ret < 0) {
		cli->infocb(OBEXFTP_EV_ERR, "info", 0, cli->infocb_data);
	} else
		cli->infocb(OBEXFTP_EV_OK, "info", 0, cli->infocb_data);
	
	return ret;
}


/**
	Send an OBEX GET with optional TYPE.
	Directories will be changed into first if split path quirk is set.

	\param cli an obexftp_client_t created by obexftp_open().
	\param type OBEX TYPE of the request
	\param localname optional file to write
	\param remotename OBEX NAME to request

	\return the result of GET request

	\note \a localname and \a remotename may be null.
 */
int obexftp_get_type(obexftp_client_t *cli, const char *type, const char *localname, const char *remotename)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);
	return_val_if_fail(remotename != NULL || type != NULL, -EINVAL);

	if (cli->buf_data) {
		DEBUG(1, "%s: Warning: buffer still active?\n", __func__);
	}

	cli->infocb(OBEXFTP_EV_RECEIVING, remotename, 0, cli->infocb_data);

	if (localname && *localname)
		cli->target_fn = strdup(localname);
	else
		cli->target_fn = NULL;

	if (OBEXFTP_USE_SPLIT_SETPATH(cli->quirks) && remotename && strchr(remotename, '/')) {
		char *basepath, *basename;
		split_file_path(remotename, &basepath, &basename);
		ret = obexftp_setpath(cli, basepath, 0);
		if(ret < 0) {
			cli->infocb(OBEXFTP_EV_ERR, basepath, 0, cli->infocb_data);
			return ret;
		}

		DEBUG(2, "%s() Getting %s -> %s (%s)\n", __func__, basename, localname, type);
		object = obexftp_build_get (cli->obexhandle, cli->connection_id, basename, type);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Getting %s -> %s (%s)\n", __func__, remotename, localname, type);
		object = obexftp_build_get (cli->obexhandle, cli->connection_id, remotename, type);
	}

	if(object == NULL)
		return -1;

	ret = cli_sync_request(cli, object);
		
	if(ret < 0) 
		cli->infocb(OBEXFTP_EV_ERR, remotename, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, remotename, 0, cli->infocb_data);

	return ret;
}


/**
	Send an custom Siemens OBEX rename request.

	\param cli an obexftp_client_t created by obexftp_open().
	\param sourcename remote filename to be renamed
	\param targetname remote target filename

	\return the result of Siemens rename request
 */
int obexftp_rename(obexftp_client_t *cli, const char *sourcename, const char *targetname)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_SENDING, sourcename, 0, cli->infocb_data);

	DEBUG(2, "%s() Moving %s -> %s\n", __func__, sourcename, targetname);

        object = obexftp_build_rename (cli->obexhandle, cli->connection_id, sourcename, targetname);
        if(object == NULL)
                return -1;
	
	cache_purge(&cli->cache, NULL);
	ret = cli_sync_request(cli, object);
		
	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, sourcename, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, sourcename, 0, cli->infocb_data);

	return ret;
}


/**
	Send an OBEX PUT with empty file name (delete).

	\param cli an obexftp_client_t created by obexftp_open().
	\param name the remote filename/foldername to be removed. 

	\return the result of the empty OBEX PUT request
 */
int obexftp_del(obexftp_client_t *cli, const char *name)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data);

	/* split path and go there first */
	if (OBEXFTP_USE_SPLIT_SETPATH(cli->quirks) && name && strchr(name, '/')) {
		char *basepath, *basename;
		split_file_path(name, &basepath, &basename);
		ret = obexftp_setpath(cli, basepath, 0);
		if(ret < 0) {
			cli->infocb(OBEXFTP_EV_ERR, basepath, 0, cli->infocb_data);
			return ret;
		}

		DEBUG(2, "%s() Deleting %s\n", __func__, basename);
		object = obexftp_build_del (cli->obexhandle, cli->connection_id, basename);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Deleting %s\n", __func__, name);
		object = obexftp_build_del (cli->obexhandle, cli->connection_id, name);
	}

	if(object == NULL)
		return -1;
	
	cache_purge(&cli->cache, NULL);
	ret = cli_sync_request(cli, object);
	
	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, name, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, name, 0, cli->infocb_data);

	return ret;
}


/**
	Send OBEX SETPATH request (multiple requests if split path flag is set).

	\param cli an obexftp_client_t created by obexftp_open().
	\param name path to change into
	\param create flag whether to create missing folders or fail

	\return the result of the OBEX SETPATH request(s).

	\note handles NULL, "", "/" and everything else correctly.
 */
int obexftp_setpath(obexftp_client_t *cli, const char *name, int create)
{
	obex_object_t *object;
	int ret = 0;
	char *copy, *tail, *p;

	return_val_if_fail(cli != NULL, -EINVAL);

	DEBUG(2, "%s() Changing to %s\n", __func__, name);

	if (OBEXFTP_USE_SPLIT_SETPATH(cli->quirks) && name && *name && strchr(name, '/')) {
		tail = copy = strdup(name);

		for (p = strchr(tail, '/'); tail; ) {
			if (p) {
				*p = '\0';
				p++;
			}
	
			cli->infocb(OBEXFTP_EV_SENDING, tail, 0, cli->infocb_data);
			DEBUG(2, "%s() Setpath \"%s\" (create:%d)\n", __func__, tail, create);
			/* try without the create flag */
			object = obexftp_build_setpath (cli->obexhandle, cli->connection_id, tail, 0);
			ret = cli_sync_request(cli, object);
			if ((ret < 0) && create) {
				/* try again with create flag set maybe? */
				object = obexftp_build_setpath (cli->obexhandle, cli->connection_id, tail, 1);
				ret = cli_sync_request(cli, object);
			}
			if (ret < 0) break;

			tail = p;
			if (p)
				p = strchr(p, '/');
			/* prevent a trailing slash from messing all up with a cd top */
			if (tail && *tail == '\0')
				break;
		}
		free (copy);
	} else {
		cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data);
		DEBUG(2, "%s() Setpath \"%s\"\n", __func__, name);
		object = obexftp_build_setpath (cli->obexhandle, cli->connection_id, name, create);
		ret = cli_sync_request(cli, object);
	}
	if (create)
		cache_purge(&cli->cache, NULL); /* no way to know where we started */

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, name, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, name, 0, cli->infocb_data);

	return ret;
}


/**
	Send an OBEX PUT, optionally with (some) SETPATHs for a local file.

	\param cli an obexftp_client_t created by obexftp_open().
	\param filename local file to send
	\param remotename remote name to write

	\return the result of the OBEX PUT (and SETPATH) request(s).

	\note Puts to filename's basename if remotename is NULL or ends with a slash.
 */
int obexftp_put_file(obexftp_client_t *cli, const char *filename, const char *remotename)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);
	return_val_if_fail(filename != NULL, -EINVAL);

	if (cli->out_data) {
		DEBUG(1, "%s: Warning: buffer still active?\n", __func__);
	}
	cli->infocb(OBEXFTP_EV_SENDING, filename, 0, cli->infocb_data);

	// TODO: if remotename ends with a slash: add basename
	if (!remotename) {
		remotename = strrchr(filename, '/');
		if (remotename)
			remotename++;
		else
			remotename = filename;
	}

	if (OBEXFTP_USE_SPLIT_SETPATH(cli->quirks) && remotename && strchr(remotename, '/')) {
		char *basepath, *basename;
		split_file_path(remotename, &basepath, &basename);
		ret = obexftp_setpath(cli, basepath, 0);
		if(ret < 0) {
			cli->infocb(OBEXFTP_EV_ERR, basepath, 0, cli->infocb_data);
			return ret;
		}

		DEBUG(2, "%s() Sending %s -> %s\n", __func__, filename, basename);
		object = build_object_from_file (cli->obexhandle, cli->connection_id, filename, basename);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Sending %s -> %s\n", __func__, filename, remotename);
		object = build_object_from_file (cli->obexhandle, cli->connection_id, filename, remotename);
	}
	
	cli->fd = open(filename, O_RDONLY | O_BINARY, 0);
	if(cli->fd < 0)
		ret = -1;
	else {
		cli->out_data = NULL; /* dont free, isnt ours */
		cache_purge(&cli->cache, NULL);
		ret = cli_sync_request(cli, object);
	}
	
	/* close(cli->fd); */

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, filename, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, filename, 0, cli->infocb_data);

	return ret;
}


/**
	Send memory data by OBEX PUT, optionally with (some) SETPATHs.

	\param cli an obexftp_client_t created by obexftp_open().
	\param data data to send
	\param size length of the data
	\param remotename remote name to write

	\return the result of the OBEX PUT (and SETPATH) request(s).

	\note A remotename must be given always.
 */
int obexftp_put_data(obexftp_client_t *cli, const char *data, int size,
		     const char *remotename)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);
	return_val_if_fail(remotename != NULL, -EINVAL);

	if (cli->out_data) {
		DEBUG(1, "%s: Warning: buffer still active?\n", __func__);
	}

	cli->infocb(OBEXFTP_EV_SENDING, remotename, 0, cli->infocb_data);

	if (OBEXFTP_USE_SPLIT_SETPATH(cli->quirks) && remotename && strchr(remotename, '/')) {
		char *basepath, *basename;
		split_file_path(remotename, &basepath, &basename);
		ret = obexftp_setpath(cli, basepath, 0);
		if(ret < 0) {
			cli->infocb(OBEXFTP_EV_ERR, basepath, 0, cli->infocb_data);
			return ret;
		}

		DEBUG(2, "%s() Sending memdata -> %s\n", __func__, basename);
		object = obexftp_build_put (cli->obexhandle, cli->connection_id, basename, size);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Sending memdata -> %s\n", __func__, remotename);
		object = obexftp_build_put (cli->obexhandle, cli->connection_id, remotename, size);
	}

	cli->out_data = data; /* memcpy would be safer */
	cli->out_size = size;
	cli->out_pos = 0;
	cli->fd = -1;
	
	cache_purge(&cli->cache, NULL);
	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, remotename, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, remotename, 0, cli->infocb_data);

	return ret;
}


/**
	Simple device discovery wrappers. USB and BT only.
 */
static char **discover_usb()
{
	char **res = NULL;
#ifdef HAVE_USB
	obex_t *handle;
	obex_interface_t* obex_intf;
	int i, interfaces_number;

	if(! (handle = OBEX_Init(OBEX_TRANS_USB, cli_obex_event, 0))) {
		DEBUG(1, "%s() OBEX_Init failed\n", __func__);
		return NULL;
	}
	interfaces_number = OBEX_FindInterfaces(handle, &obex_intf);

	res = calloc(interfaces_number + 1, sizeof(char *));
	
	for (i=0; i < interfaces_number; i++) {
		res[i] = malloc(201);
		snprintf(res[i], 200, "%d (Manufacturer: %s Product: %s Serial: %s Interface description: %s)", i,
			obex_intf[i].usb.manufacturer,
			obex_intf[i].usb.product,
			obex_intf[i].usb.serial,
		       	obex_intf[i].usb.control_interface);
	}
	/* OBEX_FreeInterfaces(handle); OpenOBEX 1.2 will crash */
	OBEX_Cleanup(handle);
#endif /* HAVE_USB */
	return res;
}


char **obexftp_discover_bt_src(const char *src)
{
#ifdef HAVE_BLUETOOTH
	return btkit_discover(src);
#else
	return NULL;
#endif /* HAVE_BLUETOOTH */
}


char *obexftp_bt_name_src(const char *addr, const char *src)
{
#ifdef HAVE_BLUETOOTH
	return btkit_getname(src, addr);
#else
	return NULL;
#endif /* HAVE_BLUETOOTH */
}


int obexftp_browse_bt_src(const char *src, const char *addr, int svclass)
{
#ifdef HAVE_BLUETOOTH
	return btkit_browse(src, addr, svclass);
#else
	return 0;
#endif /* HAVE_BLUETOOTH */
}


int obexftp_sdp_register(int svclass, int channel)
{
#ifdef HAVE_BLUETOOTH
	return btkit_register_obex(svclass, channel);
#else
	return 0;
#endif /* HAVE_BLUETOOTH */
}


int obexftp_sdp_unregister(int svclass)
{
#ifdef HAVE_BLUETOOTH
	return btkit_unregister_service(svclass);
#else
	return 0;
#endif /* HAVE_BLUETOOTH */
}


/**
	Device discovery wrapper for a named transport.

	\param transport a transport from the OBEX_TRANS_x enum.

	\return the discovery results as array of strings.

	\note USB and BT only for now.
 */
char **obexftp_discover(int transport)
{
	switch (transport)	{
	case OBEX_TRANS_BLUETOOTH:
		return obexftp_discover_bt();
	
	case OBEX_TRANS_USB:
		return discover_usb();
	
	default:
		DEBUG(1, "%s() Discovery not implemented: %d\n", __func__, transport);
		return NULL;
	}
}

