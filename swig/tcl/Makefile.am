lib_LTLIBRARIES =	obexftp.la
obexftp_la_SOURCES =	tcl_wrap.c
obexftp_la_CFLAGS =	-I$(top_srcdir) -I$(oldincludedir)/tcl
obexftp_la_LDFLAGS =	-module
obexftp_la_LIBADD =	$(top_builddir)/obexftp/libobexftp.la \
			$(top_builddir)/multicobex/libmulticobex.la \
			$(top_builddir)/bfb/libbfb.la \
			@OPENOBEX_LIBS@

if ENABLE_SWIG
tcl_wrap.c: $(srcdir)/../client.i $(srcdir)/../charmap.i
	$(SWIG) -tcl -module obexftp -I$(top_srcdir) -o $@ $(srcdir)/../client.i

MOSTLYCLEANFILES=tcl_wrap.c
endif

