INCLUDES = \
	@GLIB_CFLAGS@ \
	@OPENOBEX_CFLAGS@

LDADD = \
	@GLIB_LIBS@ \
	@OPENOBEX_LIBS@ \
	libircp.a

libircp_a_SOURCES = \
		ircp_client.c ircp_client.h \
		ircp_server.c ircp_server.h \
		ircp_io.c ircp_io.h \
		dirtraverse.c dirtraverse.h \
		debug.h ircp.h

noinst_LIBRARIES = libircp.a

s45obex_SOURCES = ircp.c

bin_PROGRAMS = s45obex 
