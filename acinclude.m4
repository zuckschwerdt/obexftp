AC_DEFUN([AC_PATH_WIN32], [
	case $host in
	*-*-mingw32*)
		EXTRA_LIBS="$EXTRA_LIBS -lws2_32"
		;;
	esac
	AC_SUBST(EXTRA_LIBS)
])


dnl
dnl Option to enable or disable IrDA support
dnl
dnl this is stupid and won't work at all.
dnl Waring: the AC_TRY_COMPILE won't work with -Werror
dnl

AC_DEFUN([IRDA_CHECK],[
AC_ARG_ENABLE([irda],
              [AC_HELP_STRING([--disable-irda],
                              [Disables irda support @<:@default=auto@:>@])],
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
dnl Waring: the AC_TRY_COMPILE won't work with -Werror
dnl

AC_DEFUN([AC_PATH_WINBT], [
	AC_CACHE_VAL(winbt_cv_found,[
		AC_CHECK_HEADERS(ws2bth.h, winbt_cv_found=yes, winbt_cv_found=no, [
				#include <winsock2.h>
		])
	])
	AC_MSG_CHECKING([for Windows Bluetooth support])
	AC_MSG_RESULT([$winbt_cv_found])
])

AC_DEFUN([AC_PATH_BSDBT], [
	AC_CACHE_CHECK([for BSD Bluetooth support], bsdbt_cv_found, [
		AC_TRY_COMPILE([
				#include <bluetooth.h>
			], [
				#ifndef BTPROTO_RFCOMM
				#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
				#endif

				bdaddr_t bdaddr;
				int family = AF_BLUETOOTH;
				int proto = BTPROTO_RFCOMM;
			], bsdbt_cv_found=yes, bsdbt_cv_found=no)
	])
	if test "${bsdbt_cv_found}" = "yes"; then
		BLUETOOTH_LIBS=-lbluetooth
		AC_CACHE_CHECK([for BSD Service Discovery support], bsdsdp_cv_found, [
			AC_TRY_COMPILE([
					#include <bluetooth.h>
					#include <sdp.h>
				], [
					struct bt_devinquiry di;
					sdp_data_t data;
				], bsdsdp_cv_found=yes, bsdsdp_cv_found=no)
		])
	fi
])

AC_DEFUN([AC_PATH_BLUEZ], [
	PKG_CHECK_MODULES(BLUETOOTH, bluez, [
		bluez_found=yes
		AC_MSG_CHECKING(for BlueZ Service Discovery support)
		AC_TRY_COMPILE([
				#include <bluetooth/sdp.h>
			],[
				sdp_list_t sdplist;
			], bluezsdp_found=yes, bluezsdp_found=no)
	], AC_MSG_RESULT(no))
])

AC_DEFUN([AC_PATH_BLUETOOTH], [
        case $host in
        *-*-linux*)
                AC_PATH_BLUEZ
                ;;
        *-*-freebsd*)
		AC_PATH_BSDBT
                ;;
        *-*-netbsd*)
		AC_PATH_BSDBT
                ;;
	*-*-mingw32*) 
		AC_PATH_WINBT 
		;;
        esac
	AC_SUBST(BLUETOOTH_CFLAGS)
	AC_SUBST(BLUETOOTH_LIBS)
])

AC_DEFUN([BLUETOOTH_CHECK],[
AC_ARG_ENABLE([bluetooth],
              [AC_HELP_STRING([--disable-bluetooth],
                              [Disables bluetooth support @<:@default=auto@:>@])],
       	      [ac_bluetooth_enabled=$enableval], [ac_bluetooth_enabled=yes])

if test "$ac_bluetooth_enabled" = yes; then
	AC_PATH_BLUETOOTH
	if test "${bsdbt_cv_found}" = "yes" -o "${bluez_found}" = "yes" -o "${winbt_cv_found}" = "yes"; then
		AC_DEFINE([HAVE_BLUETOOTH], [1], [Define if system supports Bluetooth and it's enabled])
	fi
	if test "${bsdsdp_cv_found}" = "yes" -o "${bluezsdp_found}" = "yes"; then
		AC_DEFINE([HAVE_SDP], [1], [Define if system supports Bluetooth Service Discovery])
	fi
fi
])


dnl
dnl Check for USB library
dnl Waring: the AC_TRY_COMPILE won't work with -Werror
dnl

AC_DEFUN([USB_CHECK],[
	AC_MSG_CHECKING(for USB support)

	AC_TRY_COMPILE( [
				#include <openobex/obex.h>
				#include <openobex/obex_const.h>
			],[
				obex_usb_intf_t usb_intf;
			],
				am_cv_usb_found=yes,
				am_cv_usb_found=no
			)

	if test $am_cv_usb_found = yes; then
		AC_DEFINE(HAVE_USB,1,[Define if system supports USB])
	fi

	AC_MSG_RESULT($am_cv_usb_found)
])
