#ifndef OBEX_T_H
#define OBEX_T_H

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

struct obex {
	uint16_t mtu_tx;		/* Maximum OBEX TX packet size */
        uint16_t mtu_rx;		/* Maximum OBEX RX packet size */

/* HANDLE (* void) needs to be the same size as int ... */
#ifdef _WIN32
	HANDLE fd;			/* Socket descriptor */
#else
	int fd;	        		/* Socket descriptor */
#endif
	int serverfd;
        unsigned int state;
	
	int keepserver;		/* Keep server alive */
	int filterhint;		/* Filter devices based on hint bits */
	int filterias;		/* Filter devices based on IAS entry */
#ifdef OPENOBEX_1_0_0
	uint16_t mtu_tx_max;		/* Maximum TX we can accept */
#endif

	/* truncated! */
};

#define OBEX_FD(o) (((struct obex *) (o))->fd)

#endif /* OBEX_T_H */
