/*
 * ObexFTP common Macros
 */

#ifndef _OBEXFTP_COMMON_H
#define _OBEXFTP_COMMON_H

#include <stdio.h>

#ifndef __GNUC
#define __FUNCTION__ ""
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#define return_if_fail(expr)	do { if (!(expr)) return; } while(0);
#define return_val_if_fail(expr,val)	do { if (!(expr)) return val; } while(0);

#ifdef _WIN32
#define snprintf _snprintf
#endif /* _WIN32 */

/* use 0 for production, 1 for verification, >2 for debug */
#ifndef OBEXFTP_DEBUG
#define OBEXFTP_DEBUG 0
#endif

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define	DEBUG(n, ...)	if (OBEXFTP_DEBUG >= (n)) fprintf(stderr, __VA_ARGS__)
#elif defined (__GNUC__)
#define	DEBUG(n, format...)	if (OBEXFTP_DEBUG >= (n)) fprintf (stderr, format)
#else	/* !__GNUC__ */
static void
DEBUG (int n, const char *format, ...)
{
	va_list args;
	if (OBEXFTP_DEBUG >= (n)) {
		va_start (args, format);
		fprintf (stderr, format, args);
		va_end (args);
	}
}
#endif	/* !__GNUC__ */

#endif /* _OBEXFTP_COMMON_H */
