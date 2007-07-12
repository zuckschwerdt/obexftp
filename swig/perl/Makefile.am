EXTRA_DIST =	perl_wrap.c OBEXFTP.pm
noinst_DATA =	OBEXFTP.so

if ENABLE_SWIG
perl_wrap.c: $(srcdir)/../client.i $(srcdir)/../charmap.i
	$(SWIG) -perl -module OBEXFTP -o $@ $(srcdir)/../client.i
	
MOSTLYCLEANFILES=perl_wrap.c OBEXFTP.pm OBEXFTP.bs
endif

Makefile.perl: Makefile.PL
	$(PERL) $< PREFIX=$(prefix) MAKEFILE=$@

OBEXFTP.so: perl_wrap.c Makefile.perl
	if test ! -f perl_wrap.c; then cp $(srcdir)/perl_wrap.c . ; fi
	$(MAKE) -fMakefile.perl

install-data-local: Makefile.perl
	$(MAKE) -fMakefile.perl install

# sadly there is no make uninstall for perl
#uninstall-local: Makefile.perl
#	$(MAKE) -fMakefile.perl uninstall

clean-local:
	if test -f Makefile.perl; then $(MAKE) -fMakefile.perl realclean; fi
#	rm -f Makefile.perl Makefile.perl.old
	rm -f *.so # *.o
	if test $(top_srcdir) != $(top_builddir) ; then rm -f perl_wrap.c ; fi
