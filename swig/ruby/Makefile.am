EXTRA_DIST =		extconf.rb ruby_wrap.c
noinst_DATA =		obexftp.so

if ENABLE_SWIG
ruby_wrap.c : $(srcdir)/../client.i
	$(SWIG) -ruby -module obexftp -I$(top_srcdir) -o $@ $<
endif

Makefile.ruby: extconf.rb
	$(RUBY) $< --with-obexftp-include=$(top_srcdir)

obexftp.so: ruby_wrap.c Makefile.ruby
	$(MAKE) -fMakefile.ruby

install-data-local:
	$(MAKE) -fMakefile.ruby install

