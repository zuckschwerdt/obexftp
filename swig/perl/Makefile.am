# $Id: Makefile.am,v 1.9 2005/10/13 09:20:57 d1mag Exp $

EXTRA_DIST = 	OBEXFTP.pm
noinst_PROGRAMS = OBEXFTP.pm
OBEXFTP_pm_SOURCES = perl_wrap.c

AM_CFLAGS = -I../include \
         -fexceptions \
         -Wall \
         -W \
         -Wundef \
         -Wshadow \
         -Wpointer-arith \
         -Wcast-qual \
         -Wcast-align \
         -Wconversion \
         -Wstrict-prototypes \
         -Wredundant-decls \
         -DSKIP_SEARCH_H \
         -D_FILE_OFFSET_BITS=64

AM_LDFLAGS = -lobexftp -lmulticobex -lbfb @OPENOBEX_LIBS@ @BLUETOOTH_LIBS@

if ENABLE_SWIG
perl_wrap.c: $(srcdir)/../client.i
	$(SWIG) -perl -o $@ $(srcdir)/../client.i
endif

Makefile.perl: Makefile.PL
	$(PERL) $< MAKEFILE=$@

OBEXFTP.pm: perl_wrap.c Makefile.perl
	$(MAKE) -fMakefile.perl

install-data-local:
	$(MAKE) -fMakefile.perl PREFIX=$(prefix) install

#CLEANFILES = *~ *.o perl_wrap.c OBEXFTP.pm *.PL
CLEANFILES = *~ *.o *.PL

clean-local:
	if test -f Makefile.perl; then $(MAKE) -fMakefile.perl realclean; fi
	rm -f Makefile.perl Makefile.perl.old
	rm -f $(CLEANFILES)

clean-generic:
	rm -f *~ .*~
