lib_LTLIBRARIES =		libcobexbfb.la

INCLUDES =			@GLIB_CFLAGS@ \
				@OPENOBEX_CFLAGS@

LDADD =				@GLIB_LIBS@ \
				@OPENOBEX_LIBS@

cobexbfbincludedir =		$(includedir)/cobexbfb

libcobexbfb_la_SOURCES =	cobex_bfb.c cobex_bfb.h \
				cobex_bfb_private.h \
				bfb.c bfb.h \
				crc.c crc.h \
				obex_t.h \
				debug.h

cobexbfbinclude_HEADERS =	cobex_bfb.h

libcobexbfb_la_LDFLAGS =	-version-info $(COBEXBFB_VERSION)

