
INCLUDES =			@GLIB_CFLAGS@ \
				@OPENOBEX_CFLAGS@ \
				-I.. \
				-I../includes

LDADD =				@GLIB_LIBS@ \
				@OPENOBEX_LIBS@ \
				../obexftp/libobexftp.la \
				../cobexbfb/libcobexbfb.la \
				../bfb/libbfb.la \
				../cobexpe/libcobexpe.la

obexftp_SOURCES =		obexftp_cli.c

stress_SOURCES =		stress.c

discovery_SOURCES =		discovery.c

bin_PROGRAMS =			obexftp

noinst_PROGRAMS =		stress discovery

# splint -type -predboolint -nullassign -unrecog -nullpass -I.. obexftp_cli.c

