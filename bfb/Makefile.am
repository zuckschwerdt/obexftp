lib_LTLIBRARIES =		libbfb.la

INCLUDES =			@GLIB_CFLAGS@ \
				-I../includes

LDADD =				@GLIB_LIBS@

bfbincludedir =			$(includedir)/bfb

libbfb_la_SOURCES =		bfb.c bfb.h \
				crc.c crc.h \
				bfb_io.c bfb_io.h

bfbinclude_HEADERS =		bfb.h bfb_io.h

libbfb_la_LDFLAGS =		-version-info $(BFB_VERSION)

