
INCLUDES =			@GLIB_CFLAGS@ \
				@OPENOBEX_CFLAGS@ \
				-I..

LDADD =				@GLIB_LIBS@ \
				@OPENOBEX_LIBS@ \
				../obexftp/libobexftp.la \
				../cobexbfb/libcobexbfb.la

flexmem_SOURCES =		flexmem_cli.c

stress_SOURCES =		stress.c

bin_PROGRAMS =			flexmem 

noinst_PROGRAMS =		stress 

