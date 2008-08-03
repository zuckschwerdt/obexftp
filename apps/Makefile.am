AM_CFLAGS =			@OPENOBEX_CFLAGS@ \
				-I$(top_srcdir) \
				-I$(top_srcdir)/includes

LDADD =				../obexftp/libobexftp.la \
				../multicobex/libmulticobex.la \
				../bfb/libbfb.la \
	       			@OPENOBEX_LIBS@ \
				@BLUETOOTH_LIBS@ \
				@LTLIBICONV@ \
				@EXTRA_LIBS@

obexftp_SOURCES =		obexftp.c

obexftpd_SOURCES =		obexftpd.c
stress_SOURCES =		stress.c
discovery_SOURCES =		discovery.c

bin_PROGRAMS =			obexftp obexftpd

noinst_PROGRAMS =		discovery

# splint -type -predboolint -nullassign -unrecog -nullpass -I.. obexftp_cli.c

