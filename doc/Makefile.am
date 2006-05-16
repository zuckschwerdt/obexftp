# only maintainer needs to build doc
# perl's pod2man, pod2html and pod2text needed

TOPDIR := ..
export TOPDIR
XSLBASE=http://docbook.sourceforge.net/release/xsl/current

obexftp.xml: obexftp.tmpl docproc ../obexftp/client.c
	$(top_srcdir)/doc/docproc <$< >$@

docproc_SOURCES = docproc.c
noinst_PROGRAMS = docproc

man_MANS =		obexftp.1 obexftpd.1

EXTRA_DIST =		$(man_MANS) $(noinst_DATA) \
			kernel-doc obexftp.tmpl

# transform [A-Z][A-Z]* Change*
noinst_DATA =		obexftp.pod obexftp.1.txt obexftp.1.html \
			obexftpd.pod obexftpd.1.txt obexftpd.1.html

if BUILD_DOCS

.PHONY: html

html: obexftp.html

%.html: %.xml
	xsltproc --output $@ $(XSLBASE)/xhtml/docbook.xsl $<

%.1: %.pod
	pod2man --center "ObexFTP" --release `stat --format=%y $< | cut -f 1 -d " "` --section 1 $< $@
#	txt2man -v "ObexFTP" -t "obexftp" -s 1 -r `date -I` \
#	-I ObexFTP -I gObexFtp -I vfs-obexftp $< > $@

%.1.html: %.pod
	pod2html --title "ObexFTP" --infile=$< --outfile=$@

%.1.txt: %.pod
	pod2text $< $@

#clean-local:
#	rm -f *.1

endif
