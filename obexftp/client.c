/*
 *  obexftp/client.c: ObexFTP client library
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>

#ifdef _WIN32
#define _POSIX_PATH_MAX MAX_PATH
#endif /* _WIN32 */

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif /* _WIN32 */

#ifdef HAVE_BLUETOOTH
#ifdef __FreeBSD__
#include <sys/types.h>
#include <bluetooth.h>
#else /* Linux */
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#endif /* __FreeBSD__ */
#endif

#include <openobex/obex.h>

#include "obexftp.h"
#include "client.h"
#include "object.h"
#include "obexftp_io.h"
#include "uuid.h"
#include "cache.h"

#include <common.h>


typedef struct { /* fixed to 6 bytes for now */
	uint8_t code;
	uint8_t info_len;
	uint8_t info[4];
} apparam_t;


static void dummy_info_cb(int event, const char *msg, int len, void *data)
{
	/* dummy */
}


/*
 * Normalize the path argument
 * wont turn relative paths into (most likely wrong) absolute ones
 * wont expand "../" or "./"
 */
static char *normalize_file_path(const char *name)
{
	char *p, *copy;

	return_val_if_fail(name != NULL, NULL);

	p = copy = malloc(strlen(name) + 1); /* cant be longer, can it? */
/*
	if (OBEXFTP_USE_LEADING_SLASH(quirks))
		*p++ = '/';
	while (*name == '/') name++;
*/
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


/*
 * Normalize the path argument and split into pathname and basename
 * wont turn relative paths into (most likely wrong) absolute ones
 * wont expand "../" or "./"
 * Do not use this function if there is no slash in the argument!
 */
static void split_file_path(const char *name, /*@only@*/ char **basepath, /*@only@*/ char **basename)
{
	char *p;
	const char *tail;

	return_if_fail(name != NULL);

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


/* Add more data from memory to stream. */
static int cli_fillstream_from_memory(obexftp_client_t *cli, obex_object_t *object)
{
	int actual = cli->buf_size - cli->buf_pos;
	if (actual > STREAM_CHUNK)
		actual = STREAM_CHUNK;
	DEBUG(3, "%s() Read %d bytes\n", __func__, actual);
	
	if(actual > 0) {
		/* Read was ok! */
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) &cli->buf_data[cli->buf_pos], actual, OBEX_FL_STREAM_DATA);
		cli->buf_pos += actual;
	}
	else if(actual == 0) {
		/* EOF */
		cli->buf_data = NULL; /* dont free, isnt ours */
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) &cli->buf_data[cli->buf_pos], 0, OBEX_FL_STREAM_DATAEND);
	}
	else {
		/* Error */
		cli->buf_data = NULL; /* dont free, isnt ours */
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) NULL, 0, OBEX_FL_STREAM_DATA);
	}

	return actual;
}

/* Add more data from file to stream. */
static int cli_fillstream_from_file(obexftp_client_t *cli, obex_object_t *object)
{
	int actual;
		
	DEBUG(3, "%s()\n", __func__);
	
	actual = read(cli->fd, cli->stream_chunk, STREAM_CHUNK);
	
	DEBUG(3, "%s() Read %d bytes\n", __func__, actual);
	
	if(actual > 0) {
		/* Read was ok! */
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) cli->stream_chunk, actual, OBEX_FL_STREAM_DATA);
	}
	else if(actual == 0) {
		/* EOF */
		(void) close(cli->fd);
		cli->fd = -1;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) cli->stream_chunk, 0, OBEX_FL_STREAM_DATAEND);
	}
        else {
		/* Error */
		(void) close(cli->fd);
		cli->fd = -1;
		(void) OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) NULL,
				0, OBEX_FL_STREAM_DATA);
	}

	return actual;
}


/* Save body from object or return application parameters */
static void client_done(obex_t *handle, obex_object_t *object, /*@unused@*/ int obex_cmd, /*@unused@*/ int obex_rsp)
{
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hlen;
	apparam_t *app = NULL;
	uint8_t *p;

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
			cli->infocb(OBEXFTP_EV_BODY, hv.bs, hlen, cli->infocb_data);
			DEBUG(3, "%s() Done body\n", __func__);
                        /* break; */
                }
                else if(hi == OBEX_HDR_CONNECTION) {
			DEBUG(3, "%s() Found connection number: %d\n", __func__, hv.bq4);
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
				app = (apparam_t *)hv.bs;
				 /* needed for alignment */
				memcpy(&cli->apparam_info, &app->info, sizeof(cli->apparam_info));
				cli->apparam_info = ntohl(cli->apparam_info);
				cli->infocb(OBEXFTP_EV_INFO, (void *)cli->apparam_info, 0, cli->infocb_data);
			}
			else
				DEBUG(3, "%s() Application parameters don't fit %d vs. %d.\n", __func__, hlen, sizeof(apparam_t));
                        break;
                }
                else    {
                        DEBUG(3, "%s() Skipped header %02x\n", __func__, hi);
                }
        }

        if(cli->buf_data) {
		if (cli->buf_size > 0) {
			if (cli->target_fn != NULL) {
				/* simple body writer */
				int fd;
				//fd = open_safe("", cli-> target_fn);
				fd = creat(cli-> target_fn,
					   S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
				if(fd > 0) {
					(void) write(fd, cli->buf_data, cli->buf_size);
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


/* Incoming event from OpenOBEX. */
static void cli_obex_event(obex_t *handle, obex_object_t *object, /*@unused@*/ int mode, int event, int obex_cmd, int obex_rsp)
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
		break;
	
	case OBEX_EV_STREAMEMPTY:
		if (cli->buf_data)
			(void) cli_fillstream_from_memory(cli, object);
		else
			(void) cli_fillstream_from_file(cli, object);
		break;
	
	default:
		DEBUG(1, "%s() Unknown event %d\n", __func__, event);
		break;
	}
}


/* Do an OBEX request sync. */
static int obexftp_sync(obexftp_client_t *cli)
{
	int ret;
	DEBUG(3, "%s()\n", __func__);

	/* cli->finished = FALSE; */

	while(cli->finished == FALSE) {
		ret = OBEX_HandleInput(cli->obexhandle, 20);
		DEBUG(3, "%s() OBEX_HandleInput = %d\n", __func__, ret);

		if (ret <= 0)
			return -1;
	}

	DEBUG(3, "%s() Done success=%d\n", __func__, cli->success);

	if(cli->success)
		return 1;
	else
		return -1;
}
	
static int cli_sync_request(obexftp_client_t *cli, obex_object_t *object)
{
	DEBUG(3, "%s()\n", __func__);

	if (cli->finished == FALSE)
		return -EBUSY;
	cli->finished = FALSE;
	(void) OBEX_Request(cli->obexhandle, object);

	return obexftp_sync (cli);
}
	

/* Create an obexftp client */
obexftp_client_t *obexftp_cli_open(int transport, /*const*/ obex_ctrans_t *ctrans, obexftp_info_cb_t infocb, void *infocb_data)
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

	if ( ctrans ) {
                DEBUG(2, "Do the cable-OBEX!\n");
        }
       	cli->obexhandle = OBEX_Init(transport, cli_obex_event, 0);

	if(cli->obexhandle == NULL) {
		free(cli);
		return NULL;
	}
	cli->transport = transport;

	if ( ctrans ) {
		/* OBEX_RegisterCTransport is const to ctrans ... */
                if(OBEX_RegisterCTransport(cli->obexhandle, ctrans) < 0) {
                        DEBUG(1, "Custom transport callback-registration failed\n");
                }
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
	
/* Close an obexftp client and free the resources */
void obexftp_cli_close(obexftp_client_t *cli)
{
	DEBUG(3, "%s()\n", __func__);
	return_if_fail(cli != NULL);

	OBEX_Cleanup(cli->obexhandle);
	free(cli->stream_chunk);
	free(cli);
}

/* Do connect as client */
int obexftp_cli_connect_uuid(obexftp_client_t *cli, const char *device, int port, const uint8_t uuid[])
{
	struct sockaddr_in peer;
#ifdef HAVE_BLUETOOTH
	bdaddr_t bdaddr;
#endif
#ifdef HAVE_USB
	struct usb_obex_intf *usb_intf;
#endif
	obex_object_t *object;
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
		if (inet_aton(device, &peer.sin_addr)) {
			peer.sin_port = htons(port); /* overridden with OBEX_PORT 650 anyhow */
			ret = OBEX_TransportConnect(cli->obexhandle, (struct sockaddr *) &peer,
						  sizeof(struct sockaddr_in));
			DEBUG(3, "%s() TCP %d\n", __func__, ret);
		} else
			ret = -EINVAL; /* is there a better errno? */
		break;

	case OBEX_TRANS_CUSTOM:
		ret = OBEX_TransportConnect(cli->obexhandle, NULL, 0);
		DEBUG(3, "%s() TC %d\n", __func__, ret);
		break;

#ifdef HAVE_BLUETOOTH
	case OBEX_TRANS_BLUETOOTH:
		if (!device) {
			ret = -EINVAL;
			break;
		}
		(void) str2ba(device, &bdaddr); /* what is the meaning of the return code? */
		ret = BtOBEX_TransportConnect(cli->obexhandle, BDADDR_ANY, &bdaddr, (uint8_t)port);
		DEBUG(3, "%s() BT %d\n", __func__, ret);
		break;
#endif /* HAVE_BLUETOOTH */

#ifdef HAVE_USB
	case OBEX_TRANS_USB:
		usb_intf = UsbOBEX_GetInterfaces(cli->obexhandle);
		DEBUG(3, "%s() \n", __func__);
		while (usb_intf && port > 0) {
			DEBUG(2, "%s() Skipping intf %s\n", __func__, usb_intf->device);
			usb_intf = usb_intf->next;
			port--;
		}
		if (!usb_intf) {
			DEBUG(1, "%s() %d is an invalid USB interface number\n", __func__, port);
			ret = -EINVAL; /* is there a better errno? */
		} else
			ret = UsbOBEX_TransportConnect(cli->obexhandle, usb_intf);
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
	if(OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_TARGET,
                                (obex_headerdata_t) UUID_S45,
                                sizeof(UUID_S45),
                                OBEX_FL_FIT_ONE_PACKET) < 0)    {
                DEBUG(1, "Error adding header\n");
                OBEX_ObjectDelete(cli->obexhandle, object);
                return -1;
        }
	ret = cli_sync_request(cli, object);

	if(ret < 0) {
		cli->infocb(OBEXFTP_EV_ERR, "S45 UUID", 0, cli->infocb_data);
#endif
		object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_CONNECT);
		if (uuid) {
			if(OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_TARGET,
        	        	                (obex_headerdata_t) uuid,
        		                        sizeof(UUID_FBS),
	                	                OBEX_FL_FIT_ONE_PACKET) < 0)    {
	        	        DEBUG(1, "Error adding header\n");
		                OBEX_ObjectDelete(cli->obexhandle, object);
                		return -1;
	        	}
		}
		ret = cli_sync_request(cli, object);
#ifdef COMPAT_S45
	}
#endif

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, "FBS UUID", 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, "", 0, cli->infocb_data);

	return ret;
}

/* Do disconnect as client */
int obexftp_cli_disconnect(obexftp_client_t *cli)
{
	obex_object_t *object;
	int ret;

	DEBUG(3, "%s()\n", __func__);
	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_DISCONNECTING, "", 0, cli->infocb_data);

	object = OBEX_ObjectNew(cli->obexhandle, OBEX_CMD_DISCONNECT);
	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, "disconnect", 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, "", 0, cli->infocb_data);

	/* don't -- obexftp_cli_close will handle this with OBEX_Cleanup */
	/* OBEX_TransportDisconnect(cli->obexhandle); */
	return ret;
}

/* Do an OBEX GET with app info opcode. */
int obexftp_info(obexftp_client_t *cli, uint8_t opcode)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_RECEIVING, "info", 0, cli->infocb_data);

	DEBUG(2, "%s() Retrieving info %d\n", __func__, opcode);

        object = obexftp_build_info (cli->obexhandle, opcode);
        if(object == NULL)
                return -1;
 
	ret = cli_sync_request(cli, object);
		
	if(ret < 0) {
		cli->infocb(OBEXFTP_EV_ERR, "info", 0, cli->infocb_data);
	} else
		cli->infocb(OBEXFTP_EV_OK, "info", 0, cli->infocb_data);
	
	return ret;
}

/* Do an OBEX GET with optional TYPE. localname and remotename may be null. */
/* Directories will be changed into first if split path quirk is set. */
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
		object = obexftp_build_get (cli->obexhandle, basename, type);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Getting %s -> %s (%s)\n", __func__, remotename, localname, type);
		object = obexftp_build_get (cli->obexhandle, remotename, type);
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


/* Do an OBEX rename. */
int obexftp_rename(obexftp_client_t *cli, const char *sourcename, const char *targetname)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);

	cli->infocb(OBEXFTP_EV_SENDING, sourcename, 0, cli->infocb_data);

	DEBUG(2, "%s() Moving %s -> %s\n", __func__, sourcename, targetname);

        object = obexftp_build_rename (cli->obexhandle, sourcename, targetname);
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


/* Do an OBEX PUT with empty file (delete) */
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
		object = obexftp_build_del (cli->obexhandle, basename);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Deleting %s\n", __func__, name);
		object = obexftp_build_del (cli->obexhandle, name);
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


/* Do OBEX SetPath -- handles NULL, "", "/" and everything else correctly */
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
			DEBUG(2, "%s() Setpath \"%s\"\n", __func__, tail);
			object = obexftp_build_setpath (cli->obexhandle, tail, create);
			ret = cli_sync_request(cli, object);
			if (ret < 0) break;

			tail = p;
			if (p)
				p = strchr(p, '/');
			/* prevent a trailing slash from messing all up with a cd top */
			if (*tail == '\0')
				break;
		}
		free (copy);
	} else {
		cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data);
		DEBUG(2, "%s() Setpath \"%s\"\n", __func__, name);
		object = obexftp_build_setpath (cli->obexhandle, name, create);
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

/* Do an OBEX PUT, optionally with (some) setpath. */
/* put to localname's basename if remotename is NULL or ends with a slash*/
int obexftp_put_file(obexftp_client_t *cli, const char *localname, const char *remotename)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);
	return_val_if_fail(localname != NULL, -EINVAL);

	if (cli->buf_data) {
		DEBUG(1, "%s: Warning: buffer still active?\n", __func__);
	}
	cli->infocb(OBEXFTP_EV_SENDING, localname, 0, cli->infocb_data);
/*
	if (!remotename) {
		remotename = strrchr(localname, '/');
		if (remotename)
			remotename++;
	}
*/

	if (OBEXFTP_USE_SPLIT_SETPATH(cli->quirks) && remotename && strchr(remotename, '/')) {
		char *basepath, *basename;
		split_file_path(remotename, &basepath, &basename);
		ret = obexftp_setpath(cli, basepath, 0);
		if(ret < 0) {
			cli->infocb(OBEXFTP_EV_ERR, basepath, 0, cli->infocb_data);
			return ret;
		}

		DEBUG(2, "%s() Sending %s -> %s\n", __func__, localname, basename);
		object = build_object_from_file (cli->obexhandle, localname, basename);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Sending %s -> %s\n", __func__, localname, remotename);
		object = build_object_from_file (cli->obexhandle, localname, remotename);
	}
	
	cli->fd = open(localname, O_RDONLY, 0);
	if(cli->fd < 0)
		ret = -1;
	else {
		cache_purge(&cli->cache, NULL);
		ret = cli_sync_request(cli, object);
	}
	
	/* close(cli->fd); */

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, localname, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, localname, 0, cli->infocb_data);

	return ret;
}

/* Put memory data - a remotename must be given always! */
int obexftp_put_data(obexftp_client_t *cli, const char *data, int size,
		     const char *remotename)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -EINVAL);
	return_val_if_fail(remotename != NULL, -EINVAL);

	if (cli->buf_data) {
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
		object = obexftp_build_put (cli->obexhandle, basename, size);
		free(basepath);
		free(basename);
	} else {
		DEBUG(2, "%s() Sending memdata -> %s\n", __func__, remotename);
		object = obexftp_build_put (cli->obexhandle, remotename, size);
	}

	cli->buf_data = data; /* memcpy would be safer */
	cli->buf_size = size;
	cli->buf_pos = 0;
	cli->fd = -1;
	
	cache_purge(&cli->cache, NULL);
	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, remotename, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, remotename, 0, cli->infocb_data);

	return ret;
}


/* Put file to its basename */
int obexftp_put(obexftp_client_t *cli, const char *name)
{
	const char *basename;
	basename = strrchr(name, '/');
	if (basename)
		basename++;
	else
		basename = name;

	return obexftp_put_file(cli, name, basename);
}
