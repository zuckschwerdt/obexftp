SUBDIRS = libcobexbfb

INCLUDES = \
	@GLIB_CFLAGS@ \
	@OPENOBEX_CFLAGS@

LDADD = \
	@GLIB_LIBS@ \
	@OPENOBEX_LIBS@ \
	libcobexbfb/libcobexbfb.a

libflexmem_a_SOURCES = \
		ircp_client.c ircp_client.h \
		ircp_server.c ircp_server.h \
		ircp_io.c ircp_io.h \
		dirtraverse.c dirtraverse.h \
		debug.h flexmem.h

pkginclude_HEADERS = \
		flexmem.h \
		ircp_client.h
	
lib_LIBRARIES = libflexmem.a

