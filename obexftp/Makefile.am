lib_LTLIBRARIES =		libobexftp.la

AM_CFLAGS =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

obexftpincludedir =		$(includedir)/obexftp

libobexftp_la_SOURCES =		object.c object.h \
				client.c client.h \
				obexftp_io.c obexftp_io.h \
				cache.c cache.h \
				uuid.h obexftp.h \
				unicode.c unicode.h \
				bt_kit.c bt_kit.h

obexftpinclude_HEADERS =	obexftp.h \
				client.h \
				uuid.h \
				object.h
	
libobexftp_la_LDFLAGS =		-no-undefined \
				-version-info $(OBEXFTP_VERSION)

libobexftp_la_LIBADD =		../multicobex/libmulticobex.la \
				../bfb/libbfb.la \
				@OPENOBEX_LIBS@ \
				@BLUETOOTH_LIBS@ \
				@LTLIBICONV@ \
				@EXTRA_LIBS@
