lib_LTLIBRARIES =		libobexftp.la

INCLUDES =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

obexftpincludedir =		$(includedir)/obexftp

libobexftp_la_SOURCES =		object.c object.h \
				client.c client.h \
				obexftp_io.c obexftp_io.h \
				cache.c cache.h \
				uuid.h obexftp.h \
				bt_discovery.c

obexftpinclude_HEADERS =	obexftp.h \
				client.h \
				uuid.h \
				object.h
	
libobexftp_la_LDFLAGS =		-version-info $(OBEXFTP_VERSION)

libobexftp_la_LIBADD =		@OPENOBEX_LIBS@ \
				@BLUETOOTH_LIBS@ \
				../multicobex/libmulticobex.la \
				../bfb/libbfb.la

