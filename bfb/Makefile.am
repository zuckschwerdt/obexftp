lib_LTLIBRARIES =		libbfb.la

AM_CFLAGS =			-I$(top_srcdir)/includes

bfbincludedir =			$(includedir)/bfb

libbfb_la_SOURCES =		bfb.c bfb.h \
				crc.c crc.h \
				bfb_io.c bfb_io.h

bfbinclude_HEADERS =		bfb.h bfb_io.h

libbfb_la_LDFLAGS =		-no-undefined \
				-version-info $(BFB_VERSION)

libbfb_la_LIBADD =		@EXTRA_LIBS@
