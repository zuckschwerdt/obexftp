dnl
dnl IRDA_HOOK (script-if-irda-found, failflag)
dnl
dnl if failflag is "failure" it aborts if obex is not found.
dnl

AC_DEFUN([IRDA_HOOK],[
        AC_CACHE_CHECK([for IrDA support],am_cv_irda_found,[

                AC_TRY_COMPILE([#include <sys/socket.h>
                                #include <linux/irda.h>], dnl not nice
                [struct irda_device_list l;],
                am_cv_irda_found=yes,
                am_cv_irda_found=no)])

                if test $am_cv_irda_found = yes; then
                        AC_DEFINE(HAVE_IRDA,1,[Define if system supports IrDA])

                fi
        ])

])

AC_DEFUN([IRDA_CHECK], [
        IRDA_HOOK([],failure)
])


dnl
dnl Check for Bluetooth library
dnl

AC_DEFUN([BLUETOOTH_CHECK],[
	AC_MSG_CHECKING(for Bluetooth support)

	AC_TRY_COMPILE(	[
				#ifdef __FreeBSD__
				#include <sys/types.h>
				#include <bluetooth.h>
				#else /* Linux */
				#include <sys/socket.h>
				#include <bluetooth/bluetooth.h>
				#include <bluetooth/rfcomm.h>
				#endif
			],[
				#ifdef __FreeBSD__
				bdaddr_t bdaddr;
				struct sockaddr_rfcomm addr;
				#else /* Linux */
				bdaddr_t bdaddr;
				struct sockaddr_rc addr;
				#endif
			],
				am_cv_bluetooth_found=yes,
				am_cv_bluetooth_found=no
			)

	if test $am_cv_bluetooth_found = yes; then
		AC_DEFINE(HAVE_BLUETOOTH,1,[Define if system supports Bluetooth])

		BLUETOOTH_CFLAGS=""
		BLUETOOTH_LIBS="-lbluetooth"
	fi

	AC_SUBST(BLUETOOTH_CFLAGS)
	AC_SUBST(BLUETOOTH_LIBS)
	AC_MSG_RESULT($am_cv_bluetooth_found)
])
