man_MANS =		flexmem.1

EXTRA_DIST =		flexmem.1 flexmem.1.txt flexmem.1.html \
			article.css menu.xml doc.xsl \
			ObexFS.png

# transform [A-Z][A-Z]* Change*
dist_noinst_DATA =	$(patsubst ../%,%.html,$(wildcard ../[A-Z][A-Z]*[A-Za-z0-9] ../Change*[A-Za-z0-9]))
       
#%.1: %.pod
#	pod2man --center "ObexFTP"  --release `date -I` --section 1 $< $@

%.1: %.pod
	pod2man --center "ObexFTP" --release `date -I` --section 1 $< $@
	pod2html --title "ObexFTP" --infile=$< --outfile=$@.html
	pod2text $< $@.txt
#	txt2man -v "ObexFTP" -t "flexmem" -s 1 -r `date -I` \
#	-I ObexFTP -I gObexFtp -I vfs-obexftp $< > $@
#	cat $@ | gnome-man2html2 > $@.html

%.html: %.1
	cat $< | gnome-man2html2 > $@

%.html: ../% doc.xsl menu.xml
	texthyper -pb 1 -pe 1 --xhtml --tables -H '^\s*F:\s' -H '^\s*A:\s' $< |sabcmd doc.xsl '$$filename=$*' > $@

#clean-local:
#	rm -f *.1

