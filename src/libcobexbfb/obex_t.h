
#include <glib.h>

struct obex {
	guint16 mtu_tx;			/* Maximum OBEX TX packet size */
        guint16 mtu_rx;			/* Maximum OBEX RX packet size */

	gint fd;			/* Socket descriptor */
	gint serverfd;
        guint state;
	
	gboolean keepserver;		/* Keep server alive */
	gboolean filterhint;		/* Filter devices based on hint bits */
	gboolean filterias;		/* Filter devices based on IAS entry */

	/* truncated! */
};

#define OBEX_FD(o) (((struct obex *) (o))->fd)
