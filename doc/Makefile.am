man_MANS =		flexmem.1

EXTRA_DIST =		flexmem.txt flexmem.1.html

#%.1: %.pod
#	pod2man --center "ObexFTP"  --release `date -I` --section 1 $< $@

%.1: %.txt
	pod2man --center "ObexFTP"  --release `date -I` --section 1 $< $@
	txt2man -v "ObexFTP" -t "flexmem" -s 1 -r `date -I` \
	-I ObexFTP -I gObexFtp -I vfs-obexftp $< > $@
	cat $@ | gnome-man2html2 > $@.html

%.html: %.1
	cat $< | gnome-man2html2 > $@

#clean-local:
#	rm -f *.1

