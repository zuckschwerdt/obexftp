SUBDIRS =			bfb multicobex obexftp apps doc examples swig

EXTRA_DIST =			obexftp.spec.in \
				obexftp.spec \
				includes/common.h \
				Doxyfile \
				Doxyfile.developer \
				Doxyfile.coverpage \
				config.rpath \
				tcl.m4 

pkgconfigdir =			$(libdir)/pkgconfig

pkgconfig_DATA =		obexftp.pc

# no uninstall for perl -- ignore it
distuninstallcheck_listfiles =	find . -type f \
				-not -path \*/site_perl/\*/OBEXFTP\* \
				-not -name perllocal.pod \
				-not -path \*site-packages/obexftp/\* \
				-not -path \*/site_ruby/\* \
				-print

#DISTCHECK_CONFIGURE_FLAGS =	--enable-swig --enable-perl --enable-python --enable-ruby --enable-tcl --enable-builddocs
