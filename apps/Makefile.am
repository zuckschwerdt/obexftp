EXTRA_DIST = 			perl_example.pl \
				python_example.py \
				tcl_example.tcl

INCLUDES =			@OPENOBEX_CFLAGS@ \
				@BLUETOOTH_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

LDADD =				../obexftp/libobexftp.la \
				../multicobex/libmulticobex.la \
				../bfb/libbfb.la \
				@OPENOBEX_LIBS@ \
				@BLUETOOTH_LIBS@
# need on win32:
# getopt.o getopt1.o libopenobex.a libwsock32.a

obexftp_SOURCES =		obexftp.c \
				bt_discovery.h \
				bt_discovery.c

discovery_SOURCES =		discovery.c

bin_PROGRAMS =			obexftp

noinst_PROGRAMS =		discovery

# splint -type -predboolint -nullassign -unrecog -nullpass -I.. obexftp_cli.c

