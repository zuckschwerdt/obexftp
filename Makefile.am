SUBDIRS =			bfb multicobex obexftp apps doc examples swig

ACLOCAL_AMFLAGS =		-I m4

EXTRA_DIST =			obexftp.spec.in \
				obexftp.spec \
				includes/common.h \
				Doxyfile \
				Doxyfile.developer \
				Doxyfile.coverpage \
				config.rpath

pkgconfigdir =			$(libdir)/pkgconfig

pkgconfig_DATA =		obexftp.pc

# no uninstall for perl -- ignore it
distuninstallcheck_listfiles =	find . -type f \
				-not -path \*/site_perl/\*/OBEXFTP\* \
				-not -path \*/perl/\*/auto/OBEXFTP\* \
				-not -name perllocal.pod \
				-not -path \*site-packages/obexftp\* \
				-not -path \*/site_ruby/\* \
				-print

#DISTCHECK_CONFIGURE_FLAGS =	--enable-swig --enable-perl --enable-python --enable-ruby --enable-tcl --enable-builddocs
