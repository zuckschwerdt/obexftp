EXTRA_DIST =		extconf.rb ruby_wrap.c
noinst_DATA =		obexftp.so

if ENABLE_SWIG
ruby_wrap.c : $(srcdir)/../client.i $(srcdir)/../charmap.i
	$(SWIG) -ruby -module obexftp -I$(top_srcdir) -o $@ $(srcdir)/../client.i

MOSTLYCLEANFILES=ruby_wrap.c
endif

Makefile.ruby: extconf.rb
	PREFIX=$(prefix) $(RUBY) $< --with-obexftp-include=$(top_srcdir)

obexftp.so: ruby_wrap.c Makefile.ruby
	$(MAKE) -fMakefile.ruby

install-data-local: Makefile.ruby
	$(MAKE) -j1 -fMakefile.ruby install

#uninstall
#./lib/ruby/site_ruby/1.8/i686-linux/obexftp.so

clean-local:
	if test -f Makefile.ruby; then $(MAKE) -fMakefile.ruby clean; fi
	rm -f Makefile.ruby Makefile.new
	rm -f *.o *.so *.log

