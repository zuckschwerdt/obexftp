lib_LTLIBRARIES =		libcobexpe.la

INCLUDES =			@GLIB_CFLAGS@ \
				@OPENOBEX_CFLAGS@ \
				-I../includes

LDADD =				@GLIB_LIBS@ \
				@OPENOBEX_LIBS@

cobexpeincludedir =		$(includedir)/cobexpe

libcobexpe_la_SOURCES =		cobex_pe.c cobex_pe.h \
				cobex_pe_private.h \
				obex_t.h

cobexpeinclude_HEADERS =	cobex_pe.h

libcobexpe_la_LDFLAGS =	-version-info $(COBEXPE_VERSION)

