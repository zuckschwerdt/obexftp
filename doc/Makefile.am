man_MANS =		flexmem.1

EXTRA_DIST =		flexmem.pod

%.1: %.pod
	pod2man --center "ObexFTP"  --release `date -I` --section 1 $< $@

#clean-local:
#	rm -f *.1

