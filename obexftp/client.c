
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* htons */
#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#include <openobex/obex.h>

#include "obexftp.h"
#include "client.h"
#include "object.h"
#include "obexftp_io.h"
#include "uuid.h"

#include "dirtraverse.h"
#include <common.h>

#ifdef DEBUG_TCP
#include <unistd.h>

#ifdef _WIN32
#include <winsock.h>
#else

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#endif
#endif


typedef struct { // fixed to 6 bytes for now
	uint8_t code;
	uint8_t info_len;
	uint8_t info[4];
} apparam_t;

//
// Add more data to stream.
//
static int cli_fillstream(obexftp_client_t *cli, obex_object_t *object)
{
	int actual;
	obex_headerdata_t hdd;
		
	DEBUG(3, __FUNCTION__ "()");
	
	actual = read(cli->fd, cli->buf, STREAM_CHUNK);
	
	DEBUG(3, __FUNCTION__ "() Read %d bytes", actual);
	
	if(actual > 0) {
		/* Read was ok! */
		hdd.bs = cli->buf;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hdd, actual, OBEX_FL_STREAM_DATA);
	}
	else if(actual == 0) {
		/* EOF */
		hdd.bs = cli->buf;
		close(cli->fd);
		cli->fd = -1;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hdd, 0, OBEX_FL_STREAM_DATAEND);
	}
        else {
		/* Error */
		hdd.bs = NULL;
		close(cli->fd);
		cli->fd = -1;
		OBEX_ObjectAddHeader(cli->obexhandle, object, OBEX_HDR_BODY,
				hdd, 0, OBEX_FL_STREAM_DATA);
	}

	return actual;
}


//
// Save body from object or return application parameters
//
static void client_done(obex_t *handle, obex_object_t *object, int obex_cmd, int obex_rsp)
{
        obex_headerdata_t hv;
        uint8_t hi;
        uint32_t hlen;

        const uint8_t *body = NULL;
        int body_len = 0;

	apparam_t *app = NULL;
	uint32_t info;

	obexftp_client_t *cli;

	cli = OBEX_GetUserData(handle);

        DEBUG(3, __FUNCTION__ "()\n");

	if (cli->fd > 0)
		close(cli->fd);

        while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen)) {
                if(hi == OBEX_HDR_BODY) {
			DEBUG(3, __FUNCTION__ "() Found body\n");
                        body = hv.bs;
                        body_len = hlen;
			cli->infocb(OBEXFTP_EV_BODY, body, body_len, cli->infocb_data);
			DEBUG(3, __FUNCTION__ "() Done body\n");
                        //break;
                }
                else if(hi == OBEX_HDR_CONNECTION) {
			DEBUG(3, __FUNCTION__ "() Found connection number: %d\n", hv.bq4);
		}
                else if(hi == OBEX_HDR_WHO) {
			DEBUG(3, __FUNCTION__ "() Sender identified\n");
		}
                else if(hi == OBEX_HDR_APPARAM) {
			DEBUG(3, __FUNCTION__ "() Found application parameters\n");
                        if(hlen == sizeof(apparam_t)) {
				app = (apparam_t *)hv.bs;
				 // needed for alignment
				memcpy(&info, &(app->info), sizeof(info));
				info = ntohl(info);
				cli->infocb(OBEXFTP_EV_INFO, (void *)info, 0, cli->infocb_data);
			}
			else
				DEBUG(3, __FUNCTION__ "() Application parameters don't fit %d vs. %d.\n", hlen, sizeof(apparam_t));
                        break;
                }
                else    {
                        DEBUG(3, __FUNCTION__ "() Skipped header %02x\n", hi);
                }
        }

        if(body) {
		if(cli->out_fd > 0)
			write(cli->out_fd, body, body_len);
        }
        if(app) {
		DEBUG(3, __FUNCTION__ "() Appcode %d, data (%d) %d\n",
			app->code, app->info_len, info);

        }
}


//
// Incoming event from OpenOBEX.
//
static void cli_obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
	obexftp_client_t *cli;

	cli = OBEX_GetUserData(handle);

	switch (event)	{
	case OBEX_EV_PROGRESS:
		break;
	case OBEX_EV_REQDONE:
		cli->finished = TRUE;
		if(obex_rsp == OBEX_RSP_SUCCESS)
			cli->success = TRUE;
		else
			cli->success = FALSE;
		cli->obex_rsp = obex_rsp;
		client_done(handle, object, obex_cmd, obex_rsp);
		break;
	
	case OBEX_EV_LINKERR:
		cli->finished = 1;
		cli->success = FALSE;
		break;
	
	case OBEX_EV_STREAMEMPTY:
		cli_fillstream(cli, object);
		break;
	
	default:
		DEBUG(1, __FUNCTION__ "() Unknown event %d", event);
		break;
	}
}


//
// Do an OBEX request sync.
//
int obexftp_sync(obexftp_client_t *cli)
{
	int ret;
	DEBUG(3, __FUNCTION__ "()");

	// cli->finished = FALSE;

	while(cli->finished == FALSE) {
		ret = OBEX_HandleInput(cli->obexhandle, 20);
		DEBUG(3, __FUNCTION__ "() ret = %d", ret);

		if (ret <= 0)
			return -1;
	}

	DEBUG(3, __FUNCTION__ "() Done success=%d", cli->success);

	if(cli->success)
		return 1;
	else
		return -1;
}
	
static int cli_sync_request(obexftp_client_t *cli, obex_object_t *object)
{
	DEBUG(3, __FUNCTION__ "()");

	cli->finished = FALSE;
	OBEX_Request(cli->obexhandle, object);

	return obexftp_sync (cli);
}
	

//
// Create an obexftp client
//
obexftp_client_t *obexftp_cli_open(obexftp_info_cb_t infocb, /*const*/ obex_ctrans_t *ctrans, void *infocb_data)
{
	obexftp_client_t *cli;

	DEBUG(3, __FUNCTION__ "()");
	cli = calloc (1, sizeof(obexftp_client_t));
	if(cli == NULL)
		return NULL;

	cli->infocb = infocb;
	cli->infocb_data = infocb_data;
	cli->fd = -1;

#ifdef DEBUG_TCP
	cli->obexhandle = OBEX_Init(OBEX_TRANS_INET, cli_obex_event, 0);
#else
	if ( ctrans ) {
                DEBUG(2, "Do the cable-OBEX!\n");
		cli->obexhandle = OBEX_Init(OBEX_TRANS_CUST, cli_obex_event, 0);
        }
	else
		cli->obexhandle = OBEX_Init(OBEX_TRANS_IRDA, cli_obex_event, 0);
#endif

	if(cli->obexhandle == NULL) {
		goto out_err;
	}

	if ( ctrans ) {
		/* OBEX_RegisterCTransport is const to ctrans ... */
                if(OBEX_RegisterCTransport(cli->obexhandle, ctrans) < 0) {
                        DEBUG(1, "Custom transport callback-registration failed\n");
                }
        }

	OBEX_SetUserData(cli->obexhandle, cli);
	
	/* Buffer for body */
	cli->buf = malloc(STREAM_CHUNK);
	return cli;

out_err:
	if(cli->obexhandle != NULL)
		OBEX_Cleanup(cli->obexhandle);
	free(cli);
	return NULL;
}
	
//
// Close an obexftp client
//
void obexftp_cli_close(obexftp_client_t *cli)
{
	DEBUG(3, __FUNCTION__ "()");
	return_if_fail(cli != NULL);

	OBEX_Cleanup(cli->obexhandle);
	free(cli->buf);
	free(cli);
}

//
// Do connect as client
//
int obexftp_cli_connect(obexftp_client_t *cli)
{
	obex_object_t *object;
	int ret;

	DEBUG(3, __FUNCTION__ "()");
	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_CONNECTING, "", 0, cli->infocb_data);
#ifdef DEBUG_TCP
	{
		struct sockaddr_in peer;
		u_long inaddr;
		if ((inaddr = inet_addr("127.0.0.1")) != INADDR_NONE) {
			memcpy((char *) &peer.sin_addr, (char *) &inaddr,
			      sizeof(inaddr));  
		}

		ret = OBEX_TransportConnect(cli->obexhandle, (struct sockaddr *) &peer,
					  sizeof(struct sockaddr_in));
	}
		
#else
	ret = IrOBEX_TransportConnect(cli->obexhandle, "OBEX");
	if (ret == -1 /* -ESOCKTNOSUPPORT */)
		ret = OBEX_TransportConnect(cli->obexhandle, NULL, 0);
#endif
	if (ret < 0) {
		/* could be -EBUSY or -ESOCKTNOSUPPORT */
		cli->infocb(OBEXFTP_EV_ERR, "connect", 0, cli->infocb_data);
		return -1;
	}

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

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, "target", 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, "", 0, cli->infocb_data);

	return ret;
}

//
// Do disconnect as client
//
int obexftp_cli_disconnect(obexftp_client_t *cli)
{
	obex_object_t *object;
	int ret;

	DEBUG(3, __FUNCTION__ "()");
	return_val_if_fail(cli != NULL, -1);

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

//
// Do an OBEX GET with app info opcode.
//
int obexftp_info(obexftp_client_t *cli, uint8_t opcode)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_RECEIVING, "info", 0, cli->infocb_data);

	DEBUG(2, __FUNCTION__ "() Retrieving info %d", opcode);

        object = obexftp_build_info (cli->obexhandle, opcode);
        if(object == NULL)
                return -1;
 
	ret = cli_sync_request(cli, object);
		
	if(ret < 0) 
		cli->infocb(OBEXFTP_EV_ERR, "info", 0, cli->infocb_data);
	else {
		cli->infocb(OBEXFTP_EV_OK, "info", 0, cli->infocb_data);
	}

	return ret;
}

//
// Do an OBEX GET with TYPE.
//
int obexftp_list(obexftp_client_t *cli, const char *localname, const char *remotename)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_RECEIVING, localname, 0, cli->infocb_data);

	DEBUG(2, __FUNCTION__ "() Listing %s -> %s", remotename, localname);

	cli->out_fd = STDOUT_FILENO;

        object = obexftp_build_list (cli->obexhandle, remotename);
        if(object == NULL)
                return -1;

	ret = cli_sync_request(cli, object);
		
	if(ret < 0) 
		cli->infocb(OBEXFTP_EV_ERR, localname, 0, cli->infocb_data);
	else {
		cli->infocb(OBEXFTP_EV_OK, localname, 0, cli->infocb_data);
	}

	return ret;
}


//
// Do an OBEX GET.
//
int obexftp_get(obexftp_client_t *cli, const char *localname, const char *remotename)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_RECEIVING, localname, 0, cli->infocb_data);

	DEBUG(2, __FUNCTION__ "() Getting %s -> %s", remotename, localname);

	cli->out_fd = open_safe("", localname);

        object = obexftp_build_get (cli->obexhandle, remotename);
        if(object == NULL)
                return -1;
	
	ret = cli_sync_request(cli, object);
		
	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, localname, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, localname, 0, cli->infocb_data);

	return ret;
}


//
// Do an OBEX rename.
//
int obexftp_rename(obexftp_client_t *cli, const char *sourcename, const char *targetname)
{
	obex_object_t *object = NULL;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, sourcename, 0, cli->infocb_data);

	DEBUG(2, __FUNCTION__ "() Moving %s -> %s", sourcename, targetname);

        object = obexftp_build_rename (cli->obexhandle, sourcename, targetname);
        if(object == NULL)
                return -1;
	
	ret = cli_sync_request(cli, object);
		
	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, sourcename, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, sourcename, 0, cli->infocb_data);

	return ret;
}


//
// Do an OBEX PUT with empty file (delete)
//
int obexftp_del(obexftp_client_t *cli, const char *name)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data);

	DEBUG(2, __FUNCTION__ "() Deleting %s", name);

        object = obexftp_build_del (cli->obexhandle, name);
        if(object == NULL)
                return -1;
	
	ret = cli_sync_request(cli, object);
	
	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, name, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, name, 0, cli->infocb_data);

	return ret;
}


//
// Do OBEX SetPath
//
int obexftp_setpath(obexftp_client_t *cli, const char *name, int up)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data); //FIXME

	DEBUG(2, __FUNCTION__ "() %s", name);

	object = obexftp_build_setpath (cli->obexhandle, name, up);

	ret = cli_sync_request(cli, object);

	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, name, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, name, 0, cli->infocb_data);

	return ret;
}

//
// Do an OBEX PUT.
//
static int obexftp_put_file(obexftp_client_t *cli, const char *localname, const char *remotename)
{
	obex_object_t *object;
	int ret;

	return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, localname, 0, cli->infocb_data);

	DEBUG(2, __FUNCTION__ "() Sending %s -> %s", localname, remotename);

	object = build_object_from_file (cli->obexhandle, localname, remotename);
	
	cli->fd = open(localname, O_RDONLY, 0);
	if(cli->fd < 0)
		ret = -1;
	else
		ret = cli_sync_request(cli, object);
	
	//close(cli->fd);
		
	if(ret < 0)
		cli->infocb(OBEXFTP_EV_ERR, localname, 0, cli->infocb_data);
	else
		cli->infocb(OBEXFTP_EV_OK, localname, 0, cli->infocb_data);

	return ret;
}

//
// Callback from dirtraverse.
//
static int obexftp_visit(int action, const char *name, const char *path, void *userdata)
{
	const char *remotename;
	int ret = -1;

	DEBUG(3, __FUNCTION__ "()\n");
	switch(action) {
	case VISIT_FILE:
		// Strip /'s before sending file
		remotename = strrchr(name, '/');
		if(remotename == NULL)
			remotename = name;
		else
			remotename++;
		ret = obexftp_put_file(userdata, name, remotename);
		break;

	case VISIT_GOING_DEEPER:
		ret = obexftp_setpath(userdata, name, FALSE);
		break;

	case VISIT_GOING_UP:
		ret = obexftp_setpath(userdata, "", TRUE);
		break;
	}
	DEBUG(3, __FUNCTION__ "() returning %d", ret);
	return ret;
}

//
// Put file or directory
//
int obexftp_put(obexftp_client_t *cli, const char *name)
{
	struct stat statbuf;
	char *origdir;
	int ret;
	
	/* Remember cwd */
	origdir = getcwd(NULL, 0);
	if(origdir == NULL)
		return -1;

	if(stat(name, &statbuf) == -1) {
		return -1;
	}
	
	/* This is a directory. CD into it */
	if(S_ISDIR(statbuf.st_mode)) {
		char *newrealdir = NULL;
		char *dirname;
		
		chdir(name);
		name = ".";
		
		/* Get real name of new wd, extract last part of and do setpath to it */
		newrealdir = getcwd(NULL, 0);
		dirname = strrchr(newrealdir, '/') + 1;
		if(strlen(dirname) != 0)
			obexftp_setpath(cli, dirname, FALSE);
		
		free(newrealdir);
	}
	
	ret = visit_all_files(name, obexftp_visit, cli);

	chdir(origdir);
	free(origdir);
	return ret;

}
