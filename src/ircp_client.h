#ifndef IRCP_CLIENT_H
#define IRCP_CLIENT_H

#include <openobex/obex.h>
#include "ircp.h"

typedef struct ircp_client
{
	obex_t *obexhandle;
	gboolean finished;
	gboolean success;
	gint obex_rsp;
	ircp_info_cb_t infocb;
	gint fd;
	guint8 *buf;
} ircp_client_t;


ircp_client_t *ircp_cli_open(ircp_info_cb_t infocb, obex_ctrans_t *ctrans);
void ircp_cli_close(ircp_client_t *cli);
gint ircp_cli_connect(ircp_client_t *cli);
gint ircp_cli_disconnect(ircp_client_t *cli);
gint ircp_put(ircp_client_t *cli, gchar *name);
gint ircp_del(ircp_client_t *cli, gchar *name);
gint ircp_list(ircp_client_t *cli, gchar *localname, gchar *remotename);
gint ircp_get(ircp_client_t *cli, gchar *localname, gchar *remotename);
gint ircp_rename(ircp_client_t *cli, gchar *sourcename, gchar *targetname);
	
#endif
