EXTRA_DIST = 			perl_example.pl \
				python_example.py \
				ruby_example.rb \
				tcl_example.tcl \
				ruby_obexftp.rb

INCLUDES =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

LDADD =				@OPENOBEX_LIBS@ \
				../obexftp/libobexftp.la \
				../multicobex/libmulticobex.la \
				../bfb/libbfb.la
# need on win32:
# getopt.o getopt1.o libopenobex.a libwsock32.a

obexftp_SOURCES =		obexftp.c \
				bt_discovery.h \
				bt_discovery.c

obexftpd_SOURCES =		obexftpd.c obexftp_sdp.c obexftp_sdp.h
stress_SOURCES =		stress.c
discovery_SOURCES =		discovery.c

bin_PROGRAMS =			obexftp obexftpd

noinst_PROGRAMS =		discovery

# splint -type -predboolint -nullassign -unrecog -nullpass -I.. obexftp_cli.c

