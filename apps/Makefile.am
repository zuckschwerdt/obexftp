
INCLUDES =			@OPENOBEX_CFLAGS@ \
				@BLUETOOTH_CFLAGS@ \
				-I.. \
				-I../includes

LDADD =				@OPENOBEX_LIBS@ \
				@BLUETOOTH_LIBS@ \
				../obexftp/libobexftp.la \
				../cobexbfb/libcobexbfb.la \
				../bfb/libbfb.la \
				../cobexpe/libcobexpe.la
# need on win32:
# getopt.o getopt1.o libopenobex.a libwsock32.a

obexftp_SOURCES =		obexftp_cli.c

stress_SOURCES =		stress.c

discovery_SOURCES =		discovery.c

bin_PROGRAMS =			obexftp

noinst_PROGRAMS =		stress discovery

# splint -type -predboolint -nullassign -unrecog -nullpass -I.. obexftp_cli.c

