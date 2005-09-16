dnl
dnl Option to enable or disable IrDA support
dnl
dnl this is stupid and won't work at all.
dnl

AC_DEFUN([IRDA_CHECK],[
AC_ARG_ENABLE([irda],
              [AS_HELP_STRING([--disable-irda],
                              [Disables openftp irda support @<:@default=auto@:>@])],
              [ac_irda_enabled=$enableval], [ac_irda_enabled=yes])

if test "$ac_irda_enabled" = yes; then
	AC_CACHE_CHECK([for IrDA support], am_cv_irda_found,[
		AC_TRY_COMPILE([#include <sys/socket.h>
				#include "src/irda.h"],
				[struct irda_device_list l;],
				am_cv_irda_found=yes,
				am_cv_irda_found=no)])
	if test $am_cv_irda_found = yes; then
		AC_DEFINE([HAVE_IRDA], [1], [Define if system supports IrDA and it's enabled])
	fi
fi
])


dnl
dnl Option to enable or disable Bluetooth support
dnl

AC_DEFUN([BLUETOOTH_CHECK],[  
AC_ARG_ENABLE([bluetooth],
              [AS_HELP_STRING([--disable-bluetooth],
                              [Disables openftp bluetooth support @<:@default=auto@:>@])],
       	      [ac_bluetooth_enabled=$enableval], [ac_bluetooth_enabled=yes])

if test "$ac_bluetooth_enabled" = yes; then
	AC_CACHE_CHECK([for Bluetooth support], am_cv_bluetooth_found,[

		AC_TRY_COMPILE([
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
				am_cv_bluetooth_found=no)])
	if test $am_cv_bluetooth_found = yes; then
		AC_DEFINE([HAVE_BLUETOOTH], [1], [Define if system supports Bluetooth and it's enabled])
		BLUETOOTH_CFLAGS=""
		BLUETOOTH_LIBS="-lbluetooth"
	fi
	AC_SUBST(BLUETOOTH_CFLAGS)
	AC_SUBST(BLUETOOTH_LIBS)
fi
])


dnl
dnl Check for Bluetooth SDP library
dnl

AC_DEFUN([SDPLIB_CHECK],[
	AC_MSG_CHECKING(for Bluetooth SDP support)

	AC_TRY_COMPILE(	[
				#include <bluetooth/sdp.h>
			],[
				sdp_list_t sdplist;
			],
				am_cv_sdplib_found=yes,
				am_cv_sdplib_found=no
			)

	if test $am_cv_sdplib_found = yes; then
		AC_DEFINE(HAVE_SDPLIB,1,[Define if system supports Bluetooth SDP])

	fi

	AC_MSG_RESULT($am_cv_sdplib_found)
])


dnl
dnl Check for USB library
dnl

AC_DEFUN([USB_CHECK],[
	AC_MSG_CHECKING(for USB support)

	AC_TRY_COMPILE( [
				#include <openobex/obex.h>
				#include <openobex/obex_const.h>
			],[
				usb_obex_intf_info_t usb_intf;
			],
				am_cv_usb_found=yes,
				am_cv_usb_found=no
			)

	if test $am_cv_usb_found = yes; then
		AC_DEFINE(HAVE_USB,1,[Define if system supports USB])
	fi

	AC_MSG_RESULT($am_cv_usb_found)
])

