#pkgpython_PYTHON =	pymfg.py
pkgpyexec_LTLIBRARIES =	_pymfg.la
_pymfg_la_SOURCES =	$(srcdir)/../client.i
_pymfg_la_CPPFLAGS =	$(SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/obexftp
_pymfg_la_LDFLAGS =	-module
_pymfg_la_LIBADD =     	$(top_srcdir)/obexftp/libobexftp.la \
			$(top_srcdir)/multicobex/libmulticobex.la \
			$(top_srcdir)/bfb/libbfb.la \
			@OPENOBEX_LIBS@ \
			@BLUETOOTH_LIBS@

$(srcdir)/swig_wrap.cpp : $(SWIG_SOURCES)
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/obexftp -o $@ $<
