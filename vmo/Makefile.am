
noinst_LTLIBRARIES =		libgsm.la

INCLUDES =			-I.

libgsm_la_SOURCES =		add.c            \
				code.c           \
				debug.c          \
				decode.c         \
				long_term.c      \
				lpc.c            \
				preprocess.c     \
				rpe.c            \
				gsm_destroy.c    \
				gsm_decode.c     \
				gsm_encode.c     \
				gsm_explode.c    \
				gsm_implode.c    \
				gsm_create.c     \
				gsm_print.c      \
				gsm_option.c     \
				short_term.c     \
				table.c		\
				proto.h		\
				unproto.h	\
				config.h	\
				private.h	\
				gsm.h

vmo2wav_LDADD =			-lgsm

vmo2wav_SOURCES =		vmo2wav.c

vmo2gsm_LDADD =			-lgsm

vmo2gsm_SOURCES =		vmo2gsm.c

gsm2vmo_LDADD =			-lgsm

gsm2vmo_SOURCES =		gsm2vmo.c

bin_PROGRAMS =			vmo2wav vmo2gsm gsm2vmo

