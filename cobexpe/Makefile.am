lib_LTLIBRARIES =		libcobexpe.la

INCLUDES =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

LDADD =				@OPENOBEX_LIBS@

cobexpeincludedir =		$(includedir)/cobexpe

libcobexpe_la_SOURCES =		cobex_pe.c cobex_pe.h \
				cobex_pe_private.h

cobexpeinclude_HEADERS =	cobex_pe.h

libcobexpe_la_LDFLAGS =	-version-info $(COBEXPE_VERSION)

