BUILT_SOURCES =		tcl_wrap.c
SWIG_SOURCES =		$(srcdir)/../client.i
lib_LTLIBRARIES =	obexftp.la
obexftp_la_SOURCES =	$(BUILT_SOURCES) $(SWIG_SOURCES)
obexftp_la_CPPFLAGS =	-I$(top_srcdir)/obexftp
obexftp_la_LDFLAGS =	-module
obexftp_la_LIBADD =	$(top_builddir)/obexftp/libobexftp.la \
			$(top_builddir)/multicobex/libmulticobex.la \
			$(top_builddir)/bfb/libbfb.la \
			@OPENOBEX_LIBS@ \
			@BLUETOOTH_LIBS@

tcl_wrap.c : $(SWIG_SOURCES)
	$(SWIG) -tcl -module obexftp -I$(top_srcdir)/obexftp -o $@ $<
