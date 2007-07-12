SUBDIRS =			bfb multicobex obexftp apps doc contrib swig

EXTRA_DIST =			obexftp.spec.in \
				obexftp.spec \
				includes/common.h \
				config.rpath \
				tcl.m4

# no uninstall for perl -- ignore it
distuninstallcheck_listfiles =	find . -type f \
				-not -path \*/site_perl/\*/OBEXFTP\* \
				-not -name perllocal.pod \
				-not -path \*site-packages/obexftp/\* \
				-not -path \*/site_ruby/\* \
				-print

#DISTCHECK_CONFIGURE_FLAGS =	--enable-swig --enable-perl --enable-python --enable-ruby --enable-tcl --enable-builddocs
