#ifndef OBEXFTP_CLIENT_H
#define OBEXFTP_CLIENT_H

#include <openobex/obex.h>
#include "obexftp.h"

typedef struct obexftp_client
{
	obex_t *obexhandle;
	gboolean finished;
	gboolean success;
	gint obex_rsp;
	obexftp_info_cb_t infocb;
	gpointer infocb_data;
	gint fd;
	gint out_fd;
	guint8 *buf;
} obexftp_client_t;


gint obexftp_sync(obexftp_client_t *cli);

obexftp_client_t *obexftp_cli_open(obexftp_info_cb_t infocb, const obex_ctrans_t *ctrans, gpointer infocb_data);
void obexftp_cli_close(obexftp_client_t *cli);
gint obexftp_cli_connect(obexftp_client_t *cli);
gint obexftp_cli_disconnect(obexftp_client_t *cli);

gint obexftp_put(obexftp_client_t *cli, const gchar *name);
gint obexftp_del(obexftp_client_t *cli, const gchar *name);
gint obexftp_info(obexftp_client_t *cli, guint8 opcode);
gint obexftp_list(obexftp_client_t *cli, const gchar *localname, const gchar *remotename);
gint obexftp_get(obexftp_client_t *cli, const gchar *localname, const gchar *remotename);
gint obexftp_rename(obexftp_client_t *cli, const gchar *sourcename, const gchar *targetname);

#endif // OBEXFTP_CLIENT_H
