EXTRA_DIST	= python_wrap.c __init__.py

if ENABLE_SWIG
python_wrap.c: $(srcdir)/../client.i $(srcdir)/../charmap.i
	$(SWIG) -python -module obexftp -I$(top_srcdir) -o $@ $(srcdir)/../client.i
	mv obexftp.py __init__.py

MOSTLYCLEANFILES=python_wrap.c __init__.py
endif

all-local: python_wrap.c setup.py
	if test ! -f python_wrap.c; then cp $(srcdir)/python_wrap.c . ; fi
	$(PYTHON) setup.py build

install-data-local:
	$(PYTHON) setup.py install --root="/$(DESTDIR)" --prefix="$(prefix)"

clean-local:
	if test -x "$(PYTHON)"; then $(PYTHON) setup.py clean; fi
	rm -rf build
	if test $(top_srcdir) != $(top_builddir) ; then rm -f python_wrap.c ; fi
