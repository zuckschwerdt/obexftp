SUBDIRS = libcobexbfb

INCLUDES = \
	@GLIB_CFLAGS@ \
	@OPENOBEX_CFLAGS@

LDADD = \
	@GLIB_LIBS@ \
	@OPENOBEX_LIBS@ \
	libcobexbfb/libcobexbfb.a

libflexmem_a_SOURCES = \
		client.c client.h \
		io.c io.h \
		dirtraverse.c dirtraverse.h \
		debug.h flexmem.h

pkginclude_HEADERS = \
		flexmem.h \
		client.h
	
lib_LIBRARIES = libflexmem.a

