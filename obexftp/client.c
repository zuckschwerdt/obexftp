#include <glib.h>
#include <openobex/obex.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "obexftp.h"
#include "client.h"
#include "object.h"
#include "obexftp_io.h"
#include "uuid.h"

#include "dirtraverse.h"
#include <g_debug.h>

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
	guint8 code;
	guint8 info_len;
	guint8 info[4];
} apparam_t;

//
// Add more data to stream.
//
static gint cli_fillstream(obexftp_client_t *cli, obex_object_t *object)
{
	gint actual;
	obex_headerdata_t hdd;
		
	g_debug(G_GNUC_FUNCTION "()");
	
	actual = read(cli->fd, cli->buf, STREAM_CHUNK);
	
	g_debug(G_GNUC_FUNCTION "() Read %d bytes", actual);
	
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
static void client_done(obex_t *handle, obex_object_t *object, gint obex_cmd, gint obex_rsp)
{
        obex_headerdata_t hv;
        guint8 hi;
        guint32 hlen;

        const guint8 *body = NULL;
        gint body_len = 0;

	apparam_t *app = NULL;
	guint32 info;

	obexftp_client_t *cli;

	cli = OBEX_GetUserData(handle);

        g_debug(G_GNUC_FUNCTION "()\n");

	if (cli->fd > 0)
		close(cli->fd);

        while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen)) {
                if(hi == OBEX_HDR_BODY) {
			g_debug(G_GNUC_FUNCTION "() Found body\n");
                        body = hv.bs;
                        body_len = hlen;
			cli->infocb(OBEXFTP_EV_BODY, body, body_len, cli->infocb_data);
			g_debug(G_GNUC_FUNCTION "() Done body\n");
                        //break;
                }
                else if(hi == OBEX_HDR_CONNECTION) {
			g_debug(G_GNUC_FUNCTION "() Found connection number: %d\n", hv.bq4);
		}
                else if(hi == OBEX_HDR_WHO) {
			g_debug(G_GNUC_FUNCTION "() Sender identified\n");
		}
                else if(hi == OBEX_HDR_APPARAM) {
			g_debug(G_GNUC_FUNCTION "() Found application parameters\n");
                        if(hlen == sizeof(apparam_t)) {
				app = (apparam_t *)hv.bs;
				 // needed for alignment
				memcpy(&info, &(app->info), sizeof(info));
				info = g_ntohl(info);
				cli->infocb(OBEXFTP_EV_INFO, GUINT_TO_POINTER (info), 0, cli->infocb_data);
			}
			else
				g_debug(G_GNUC_FUNCTION "() Application parameters don't fit %d vs. %d.\n", hlen, sizeof(apparam_t));
                        break;
                }
                else    {
                        g_debug(G_GNUC_FUNCTION "() Skipped header %02x\n", hi);
                }
        }

        if(body) {
		if(cli->out_fd > 0)
			write(cli->out_fd, body, body_len);
        }
        if(app) {
		g_debug(G_GNUC_FUNCTION "() Appcode %d, data (%d) %d\n",
			app->code, app->info_len, info);

        }
}


//
// Incoming event from OpenOBEX.
//
static void cli_obex_event(obex_t *handle, obex_object_t *object, gint mode, gint event, gint obex_cmd, gint obex_rsp)
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
		g_warning(G_GNUC_FUNCTION "() Unknown event %d", event);
		break;
	}
}


//
// Do an OBEX request sync.
//
gint obexftp_sync(obexftp_client_t *cli)
{
	gint ret;
	g_debug(G_GNUC_FUNCTION "()");

	// cli->finished = FALSE;

	while(cli->finished == FALSE) {
		ret = OBEX_HandleInput(cli->obexhandle, 20);
		g_debug(G_GNUC_FUNCTION "() ret = %d", ret);

		if (ret <= 0)
			return -1;
	}

	g_debug(G_GNUC_FUNCTION "() Done success=%d", cli->success);

	if(cli->success)
		return 1;
	else
		return -1;
}
	
static gint cli_sync_request(obexftp_client_t *cli, obex_object_t *object)
{
	g_debug(G_GNUC_FUNCTION "()");

	cli->finished = FALSE;
	OBEX_Request(cli->obexhandle, object);

	return obexftp_sync (cli);
}
	

//
// Create an obexftp client
//
obexftp_client_t *obexftp_cli_open(obexftp_info_cb_t infocb, /*const*/ obex_ctrans_t *ctrans, gpointer infocb_data)
{
	obexftp_client_t *cli;

	g_debug(G_GNUC_FUNCTION "()");
	cli = g_new0(obexftp_client_t, 1);
	if(cli == NULL)
		return NULL;

	cli->infocb = infocb;
	cli->infocb_data = infocb_data;
	cli->fd = -1;

#ifdef DEBUG_TCP
	cli->obexhandle = OBEX_Init(OBEX_TRANS_INET, cli_obex_event, 0);
#else
	if ( ctrans ) {
                g_info("Do the cable-OBEX!\n");
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
                        g_warning("Custom transport callback-registration failed\n");
                }
        }

	OBEX_SetUserData(cli->obexhandle, cli);
	
	/* Buffer for body */
	cli->buf = g_malloc(STREAM_CHUNK);
	return cli;

out_err:
	if(cli->obexhandle != NULL)
		OBEX_Cleanup(cli->obexhandle);
	g_free(cli);
	return NULL;
}
	
//
// Close an obexftp client
//
void obexftp_cli_close(obexftp_client_t *cli)
{
	g_debug(G_GNUC_FUNCTION "()");
	g_return_if_fail(cli != NULL);

	OBEX_Cleanup(cli->obexhandle);
	g_free(cli->buf);
	g_free(cli);
}

//
// Do connect as client
//
gint obexftp_cli_connect(obexftp_client_t *cli)
{
	obex_object_t *object;
	int ret;

	g_debug(G_GNUC_FUNCTION "()");
	g_return_val_if_fail(cli != NULL, -1);

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
                g_warning("Error adding header\n");
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
gint obexftp_cli_disconnect(obexftp_client_t *cli)
{
	obex_object_t *object;
	int ret;

	g_debug(G_GNUC_FUNCTION "()");
	g_return_val_if_fail(cli != NULL, -1);

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
gint obexftp_info(obexftp_client_t *cli, guint8 opcode)
{
	obex_object_t *object = NULL;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_RECEIVING, "info", 0, cli->infocb_data);

	g_info(G_GNUC_FUNCTION "() Retrieving info %d", opcode);

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
gint obexftp_list(obexftp_client_t *cli, const gchar *localname, const gchar *remotename)
{
	obex_object_t *object = NULL;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_RECEIVING, localname, 0, cli->infocb_data);

	g_info(G_GNUC_FUNCTION "() Listing %s -> %s", remotename, localname);

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
gint obexftp_get(obexftp_client_t *cli, const gchar *localname, const gchar *remotename)
{
	obex_object_t *object = NULL;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_RECEIVING, localname, 0, cli->infocb_data);

	g_info(G_GNUC_FUNCTION "() Getting %s -> %s", remotename, localname);

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
gint obexftp_rename(obexftp_client_t *cli, const gchar *sourcename, const gchar *targetname)
{
	obex_object_t *object = NULL;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, sourcename, 0, cli->infocb_data);

	g_info(G_GNUC_FUNCTION "() Moving %s -> %s", sourcename, targetname);

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
gint obexftp_del(obexftp_client_t *cli, const gchar *name)
{
	obex_object_t *object;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data);

	g_info(G_GNUC_FUNCTION "() Deleting %s", name);

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
gint obexftp_setpath(obexftp_client_t *cli, const gchar *name, gboolean up)
{
	obex_object_t *object;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, name, 0, cli->infocb_data); //FIXME

	g_info(G_GNUC_FUNCTION "() %s", name);

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
static gint obexftp_put_file(obexftp_client_t *cli, const gchar *localname, const gchar *remotename)
{
	obex_object_t *object;
	int ret;

	g_return_val_if_fail(cli != NULL, -1);

	cli->infocb(OBEXFTP_EV_SENDING, localname, 0, cli->infocb_data);

	g_info(G_GNUC_FUNCTION "() Sending %s -> %s", localname, remotename);

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
static gint obexftp_visit(gint action, const gchar *name, const gchar *path, gpointer userdata)
{
	const gchar *remotename;
	gint ret = -1;

	g_debug(G_GNUC_FUNCTION "()\n");
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
	g_debug(G_GNUC_FUNCTION "() returning %d", ret);
	return ret;
}

//
// Put file or directory
//
gint obexftp_put(obexftp_client_t *cli, const gchar *name)
{
	struct stat statbuf;
	gchar *origdir;
	gint ret;
	
	/* Remember cwd */
	origdir = getcwd(NULL, 0);
	if(origdir == NULL)
		return -1;

	if(stat(name, &statbuf) == -1) {
		return -1;
	}
	
	/* This is a directory. CD into it */
	if(S_ISDIR(statbuf.st_mode)) {
		gchar *newrealdir = NULL;
		gchar *dirname;
		
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
