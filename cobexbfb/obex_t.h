
#include <glib.h>

#ifdef _WIN32
#include <windows.h>
#endif

struct obex {
	guint16 mtu_tx;			/* Maximum OBEX TX packet size */
        guint16 mtu_rx;			/* Maximum OBEX RX packet size */

/* HANDLE (* void) needs to be the same size as gint ... */
#ifdef _WIN32
	HANDLE fd;			/* Socket descriptor */
#else
	gint fd;			/* Socket descriptor */
#endif
	gint serverfd;
        guint state;
	
	gboolean keepserver;		/* Keep server alive */
	gboolean filterhint;		/* Filter devices based on hint bits */
	gboolean filterias;		/* Filter devices based on IAS entry */

	/* truncated! */
};

#define OBEX_FD(o) (((struct obex *) (o))->fd)
