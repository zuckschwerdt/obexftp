# only maintainer needs to build doc
# asciidoc, docbook2X needed

EXTRA_DIST =		$(man_MANS) $(noinst_DATA)

man_MANS =			obexftp.1 obexftpd.1

noinst_DATA =		obexftp.1.txt obexftp.1.html \
					obexftpd.1.txt obexftpd.1.html

if BUILD_DOCS

SUFFIXES = .1 .1.txt .1.html

.1.txt.1:
	asciidoc --doctype=manpage --backend=docbook $<
	docbook2man $*.1.xml

.1.txt.1.html:
	asciidoc --doctype=manpage --backend=xhtml11 $<

CLEANFILES =		obexftp.1.xml obexftp.1 obexftp.1.html \
					obexftpd.1.xml obexftpd.1 obexftpd.1.html

endif
