//#define DEBUG_TCP 1

#define OBEXFTP_DEBUG 4
#ifdef OBEXFTP_DEBUG
#define DEBUG(n, args...) if(n <= OBEXFTP_DEBUG) g_print(args)
#else
#define DEBUG(n, ...)
#endif // OBEXFTP_DEBUG
