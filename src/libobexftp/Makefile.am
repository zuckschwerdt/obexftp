INCLUDES = \
	@GLIB_CFLAGS@ \
	@OPENOBEX_CFLAGS@

LDADD = \
	@GLIB_LIBS@ \
	@OPENOBEX_LIBS@

libobexftp_a_SOURCES = \
	object.c object.h \
	client.c client.h \
	io.c io.h \
	dirtraverse.c dirtraverse.h \
	debug.h obexftp.h

pkginclude_HEADERS = \
	obexftp.h \
	client.h \
	object.h
	
lib_LIBRARIES = libobexftp.a

