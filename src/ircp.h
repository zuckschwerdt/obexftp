#ifndef IRCP_H
#define IRCP_H

#include <glib.h>

typedef void (*ircp_info_cb_t)(gint event, gchar *param);

enum {
	IRCP_EV_ERRMSG,

	IRCP_EV_OK,
	IRCP_EV_ERR,

	IRCP_EV_CONNECTING,
	IRCP_EV_DISCONNECTING,
	IRCP_EV_SENDING,

	IRCP_EV_LISTENING,
	IRCP_EV_CONNECTIND,
	IRCP_EV_DISCONNECTIND,
	IRCP_EV_RECEIVING,
};

/* Number of bytes passed at one time to OBEX */
#define STREAM_CHUNK 4096

#endif
