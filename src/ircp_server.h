#ifndef IRCP_SERVER_H
#define IRCP_SERVER_H

#include <glib.h>
#include <openobex/obex.h>
#include "ircp.h"

typedef struct ircp_server
{
	obex_t *obexhandle;
	gboolean finished;
	gboolean success;
	gchar *inbox;
	ircp_info_cb_t infocb;
	gint fd;
	gint dirdepth;

} ircp_server_t;

gint ircp_srv_receive(ircp_server_t *srv, obex_object_t *object, gboolean finished);
gint ircp_srv_setpath(ircp_server_t *srv, obex_object_t *object);

ircp_server_t *ircp_srv_open(ircp_info_cb_t infocb);
void ircp_srv_close(ircp_server_t *srv);
gint ircp_srv_recv(ircp_server_t *srv, gchar *inbox);


#endif
