SUBDIRS = libcobexbfb libobexftp

INCLUDES = \
	@GLIB_CFLAGS@ \
	@OPENOBEX_CFLAGS@

LDADD = \
	@GLIB_LIBS@ \
	@OPENOBEX_LIBS@ \
	libobexftp/libobexftp.a \
	libcobexbfb/libcobexbfb.a

flexmem_SOURCES = flexmem_cli.c

bin_PROGRAMS = flexmem 
