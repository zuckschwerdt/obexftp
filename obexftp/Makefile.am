lib_LTLIBRARIES =		libobexftp.la

INCLUDES =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

LDADD =				@OPENOBEX_LIBS@

obexftpincludedir =		$(includedir)/obexftp

libobexftp_la_SOURCES =		object.c object.h \
				client.c client.h \
				obexftp_io.c obexftp_io.h \
				dirtraverse.c dirtraverse.h \
				uuid.h obexftp.h

obexftpinclude_HEADERS =	obexftp.h \
				client.h \
				object.h
	
libobexftp_la_LDFLAGS =		-version-info $(OBEXFTP_VERSION)

