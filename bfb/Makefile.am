lib_LTLIBRARIES =		libbfb.la

INCLUDES =			@GLIB_CFLAGS@

LDADD =				@GLIB_LIBS@

bfbincludedir =			$(includedir)/bfb

libbfb_la_SOURCES =		bfb.c bfb.h \
				crc.c crc.h \
				bfb_io.c bfb_io.h \
				debug.h

bfbinclude_HEADERS =		bfb.h bfb_io.h

libbfb_la_LDFLAGS =		-version-info $(BFB_VERSION)

