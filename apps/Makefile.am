
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

flexmem_SOURCES =		flexmem_cli.c

stress_SOURCES =		stress.c

discovery_SOURCES =		discovery.c

bin_PROGRAMS =			flexmem

noinst_PROGRAMS =		stress discovery

# splint -type -predboolint -nullassign -unrecog -nullpass -I.. flexmem_cli.c

