lib_LTLIBRARIES =		libmulticobex.la

AM_CFLAGS =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

multicobexincludedir =		$(includedir)/multicobex

libmulticobex_la_SOURCES =	multi_cobex.c multi_cobex.h \
				multi_cobex_private.h

multicobexinclude_HEADERS =	multi_cobex.h

libmulticobex_la_LDFLAGS =	-no-undefined \
				-version-info $(MULTICOBEX_VERSION)

libmulticobex_la_LIBADD =	@OPENOBEX_LIBS@ \
				../bfb/libbfb.la \
				@EXTRA_LIBS@
