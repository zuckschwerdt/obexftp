dnl
dnl Check for Bluetooth library
dnl

AC_DEFUN([BLUETOOTH_CHECK],[
	AC_MSG_CHECKING(for Bluetooth support)

	AC_TRY_COMPILE(	[	#include <sys/socket.h>
				#include <bluetooth/bluetooth.h>
				#include <bluetooth/rfcomm.h>
			],[
				bdaddr_t bdaddr;
				 struct sockaddr_rc addr;
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
