#ifndef FLEXMEM_CLIENT_H
#define FLEXMEM_CLIENT_H

#include <openobex/obex.h>
#include "flexmem.h"

typedef struct ircp_client
{
	obex_t *obexhandle;
	gboolean finished;
	gboolean success;
	gint obex_rsp;
	ircp_info_cb_t infocb;
	gint fd;
	gint out_fd;
	guint8 *buf;
} ircp_client_t;


gint flexmem_sync(ircp_client_t *cli);

ircp_client_t *flexmem_cli_open(ircp_info_cb_t infocb, gchar *tty);
void flexmem_cli_close(ircp_client_t *cli);
gint flexmem_cli_connect(ircp_client_t *cli);
gint flexmem_cli_disconnect(ircp_client_t *cli);

gint flexmem_put(ircp_client_t *cli, gchar *name);
gint flexmem_del(ircp_client_t *cli, gchar *name);
gint flexmem_info(ircp_client_t *cli, guint8 opcode);
gint flexmem_list(ircp_client_t *cli, gchar *localname, gchar *remotename);
gint flexmem_get(ircp_client_t *cli, gchar *localname, gchar *remotename);
gint flexmem_rename(ircp_client_t *cli, gchar *sourcename, gchar *targetname);

obex_object_t *flexmem_build_info (obex_t obex, guint8 opcode);
obex_object_t *flexmem_build_list (obex_t obex, gchar *name);
obex_object_t *flexmem_build_get (obex_t obex, gchar *name);
obex_object_t *flexmem_build_rename (obex_t obex, gchar *source, gchar *target);
obex_object_t *flexmem_build_del (obex_t obex, gchar *name);
obex_object_t *flexmem_build_setpath (obex_t obex, gchar *name, gboolean up);
obex_object_t *flexmem_build_put (obex_t obex, gchar *name);
	
#endif // FLEXMEM_CLIENT_H
