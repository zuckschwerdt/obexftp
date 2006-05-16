pkgpython_PYTHON =	__init__.py
pkgpyexec_LTLIBRARIES =	_obexftp.la
_obexftp_la_SOURCES =	python_wrap.c
_obexftp_la_CPPFLAGS =	$(SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/obexftp
_obexftp_la_LDFLAGS =	-module
_obexftp_la_LIBADD =	$(top_builddir)/obexftp/libobexftp.la \
			$(top_builddir)/multicobex/libmulticobex.la \
			$(top_builddir)/bfb/libbfb.la \
			@OPENOBEX_LIBS@

if ENABLE_SWIG
python_wrap.c : $(srcdir)/../client.i
	$(SWIG) $(SWIG_PYTHON_OPT) -module obexftp -I$(top_srcdir)/obexftp -o $@ $<
	mv obexftp.py __init__.py
endif
