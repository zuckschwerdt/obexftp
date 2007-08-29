/**
	\file includes/common.h
	ObexFTP common macros and debugging.
	ObexFTP library - language bindings for OBEX file transfer.
 */

#ifndef _OBEXFTP_COMMON_H
#define _OBEXFTP_COMMON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

/* these are not asserts! dont define to nothing */
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

#if OBEXFTP_DEBUG > 4
#define DEBUGBUFFER(b,l) do { \
	int i; \
	for (i=0; i < (l); i++) \
		fprintf (stderr, "%02x ", ((uint8_t *)(b))[i]); \
	fprintf (stderr, "\n"); \
} while (0)
#else
#define DEBUGBUFFER(b,l) do { } while (0)
#endif

#endif /* _OBEXFTP_COMMON_H */
