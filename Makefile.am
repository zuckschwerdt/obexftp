SUBDIRS =			bfb multicobex obexftp apps doc contrib swig

EXTRA_DIST =			obexftp.spec.in \
				obexftp.spec \
				includes/common.h \
				config.rpath \
				tcl.m4

# no uninstall for perl -- ignore it
distuninstallcheck_listfiles = \
            find -type f -not -path \*/OBEXFTP\* -not -name perllocal.pod

#DISTCHECK_CONFIGURE_FLAGS =	--enable-swig --enable-perl --enable-python --enable-ruby --enable-tcl --enable-builddocs
