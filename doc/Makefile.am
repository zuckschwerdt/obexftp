man_MANS =		obexftp.1

EXTRA_DIST =		$(man_MANS) $(noinst_DATA) \
			obexftp.pod obexftp.1.txt obexftp.1.html \
			article.css menu.xml doc.xsl \
			ObexFS.png

# transform [A-Z][A-Z]* Change*
noinst_DATA =	$(patsubst ../%,%.html,$(wildcard ../[A-Z][A-Z]*[A-Za-z0-9] ../Change*[A-Za-z0-9]))

%.1: %.pod
	pod2man --center "ObexFTP" --release `date -I` --section 1 $< $@
	pod2html --title "ObexFTP" --infile=$< --outfile=$@.html
	pod2text $< $@.txt
#	txt2man -v "ObexFTP" -t "obexftp" -s 1 -r `date -I` \
#	-I ObexFTP -I gObexFtp -I vfs-obexftp $< > $@

%.1.html: %.pod
	pod2html --title "ObexFTP" --infile=$< --outfile=$@.html

%.1.txt: %.pod
	pod2text $< $@.txt

#%.1.html: %.1
#	cat $< | gnome-man2html2 > $@

%.html: ../% doc.xsl menu.xml
	txt2html -pb 1 -pe 1 --bold_delimiter "" --xhtml --tables -H '^\s*F:\s' -H '^\s*A:\s' $< |sabcmd doc.xsl '$$filename=$*' > $@

#clean-local:
#	rm -f *.1

