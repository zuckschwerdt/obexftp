/*
 * Most likely you want to define a handler to discard debug and info logs:
 *
 * void g_log_null_handler (const gchar *log_domain,
 *			    GLogLevelFlags log_level,
 *			    const gchar *message,
 *			    gpointer user_data) { }
 *
 * g_log_set_handler (NULL,
 *		      G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO,
 *		      g_log_null_handler,
 *		      NULL);
 *
 */

#ifndef __G_DEBUG_H__
#define __G_DEBUG_H__

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define	g_info(...)	g_log (G_LOG_DOMAIN,         \
			       G_LOG_LEVEL_INFO,    \
			       __VA_ARGS__)
#define	g_debug(...)	g_log (G_LOG_DOMAIN,         \
			       G_LOG_LEVEL_DEBUG,  \
			       __VA_ARGS__)
#elif defined (__GNUC__)
#define	g_info(format...)	g_log (G_LOG_DOMAIN,         \
				       G_LOG_LEVEL_INFO,    \
				       format)
#define	g_debug(format...)	g_log (G_LOG_DOMAIN,         \
				       G_LOG_LEVEL_DEBUG,  \
				       format)
#else	/* !__GNUC__ */
static void
g_info (const gchar *format,
	 ...)
{
  va_list args;
  va_start (args, format);
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format, args);
  va_end (args);
}
static void
g_debug (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format, args);
  va_end (args);
}
#endif	/* !__GNUC__ */

#endif /* __G_DEBUG_H__ */
