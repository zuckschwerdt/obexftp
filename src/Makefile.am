SUBDIRS = libflexmem

INCLUDES = \
	@GLIB_CFLAGS@ \
	@OPENOBEX_CFLAGS@

LDADD = \
	@GLIB_LIBS@ \
	@OPENOBEX_LIBS@ \
	libflexmem/libflexmem.a \
	libflexmem/libcobexbfb/libcobexbfb.a

flexmem_SOURCES = flexmem_cli.c

bin_PROGRAMS = flexmem 
