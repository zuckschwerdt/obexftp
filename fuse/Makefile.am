## Process this file with automake to produce Makefile.in

if BUILD_FUSE
AM_CFLAGS = 			@FUSE_CFLAGS@ \
				-I$(top_builddir) \
				-I$(top_srcdir)

LDADD =				@FUSE_LIBS@ \
				../obexftp/libobexftp.la

obexfs_SOURCES =		obexfs.c

obexautofs_SOURCES =		obexautofs.c

bin_PROGRAMS =			obexfs obexautofs
endif