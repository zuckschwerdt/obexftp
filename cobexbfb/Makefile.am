lib_LTLIBRARIES =		libcobexbfb.la

INCLUDES =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

LDADD =				@OPENOBEX_LIBS@ \
				../bfb/libbfb.la

cobexbfbincludedir =		$(includedir)/cobexbfb

libcobexbfb_la_SOURCES =	cobex_bfb.c cobex_bfb.h \
				cobex_bfb_private.h

cobexbfbinclude_HEADERS =	cobex_bfb.h

libcobexbfb_la_LDFLAGS =	-version-info $(COBEXBFB_VERSION)

