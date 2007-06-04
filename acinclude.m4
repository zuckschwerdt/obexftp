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
dnl Waring: the AC_TRY_COMPILE won't work with -Werror
dnl

AC_DEFUN([AC_PATH_WINBT], [ 
	AC_CACHE_VAL(winbt_found,[ 
		AC_CHECK_HEADERS(ws2bth.h, winbt_found=yes, winbt_found=no, [ 
				#include <winsock2.h> 
		]) 
	]) 
	AC_MSG_CHECKING([for Windows Bluetooth support]) 
	AC_MSG_RESULT([$winbt_found]) 
])

AC_DEFUN([AC_PATH_NETBSDBT], [
	AC_CACHE_CHECK([for NetBSD Bluetooth support], netbsdbt_found, [
		AC_TRY_COMPILE([
				#include <bluetooth.h>
			], [
				struct sockaddr_bt *bt;
			], netbsdbt_found=yes, netbsdbt_found=no)
	])
])

AC_DEFUN([AC_PATH_FREEBSDBT], [
	AC_CACHE_CHECK([for FreeBSD Bluetooth support], freebsdbt_found, [
		AC_TRY_COMPILE([
				#include <bluetooth.h>
			], [
				struct sockaddr_rfcomm *rfcomm;
			], freebsdbt_found=yes, freebsdbt_found=no)
	])
])

AC_DEFUN([AC_PATH_BLUEZ], [
	PKG_CHECK_MODULES(BLUETOOTH, bluez, bluez_found=yes, AC_MSG_RESULT(no)) 
])

AC_DEFUN([AC_PATH_BLUETOOTH], [
        case $host in
        *-*-linux*)
                AC_PATH_BLUEZ
                ;;
        *-*-freebsd*)
                AC_PATH_FREEBSDBT
                ;;
        *-*-netbsd*)
                AC_PATH_NETBSDBT
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
                              [Disables openftp bluetooth support @<:@default=auto@:>@])],
       	      [ac_bluetooth_enabled=$enableval], [ac_bluetooth_enabled=yes])

if test "$ac_bluetooth_enabled" = yes; then
       	AC_PATH_BLUETOOTH
	if test "${netbsdbt_found}" = "yes" -o "${freebsdbt_found}" = "yes" -o "${bluez_found}" = "yes" -o "${winbt_found}" = "yes"; then
		AC_DEFINE([HAVE_BLUETOOTH], [1], [Define if system supports Bluetooth and it's enabled])
	fi
fi
])


dnl
dnl Check for Bluetooth SDP library
dnl Waring: the AC_TRY_COMPILE won't work with -Werror
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

dnl
dnl From autoconf-archive, should be in macros dir
dnl
AC_DEFUN([AC_PROG_SWIG],[
        AC_PATH_PROG([SWIG],[swig])
        if test -z "$SWIG" ; then
                AC_MSG_WARN([cannot find 'swig' program. You should look at http://www.swig.org])
                SWIG='echo "Error: SWIG is not installed. You should look at http://www.swig.org" ; false'
        elif test -n "$1" ; then
                AC_MSG_CHECKING([for SWIG version])
                [swig_version=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
                AC_MSG_RESULT([$swig_version])
                if test -n "$swig_version" ; then
                        # Calculate the required version number components
                        [required=$1]
                        [required_major=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_major" ; then
                                [required_major=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_minor=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_minor" ; then
                                [required_minor=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_patch=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_patch" ; then
                                [required_patch=0]
                        fi
                        # Calculate the available version number components
                        [available=$swig_version]
                        [available_major=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_major" ; then
                                [available_major=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_minor=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_minor" ; then
                                [available_minor=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_patch=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_patch" ; then
                                [available_patch=0]
                        fi
                        if test $available_major -ne $required_major \
                                -o $available_minor -ne $required_minor \
                                -o $available_patch -lt $required_patch ; then
                                AC_MSG_WARN([SWIG version >= $1 is required.  You have $swig_version.  You should look at http://www.swig.org])
                                SWIG='echo "Error: SWIG version >= $1 is required.  You have '"$swig_version"'.  You should look at http://www.swig.org" ; false'
                        else
                                AC_MSG_NOTICE([SWIG executable is '$SWIG'])
                                SWIG_LIB=`$SWIG -swiglib`
                                AC_MSG_NOTICE([SWIG library directory is '$SWIG_LIB'])
                        fi
                else
                        AC_MSG_WARN([cannot determine SWIG version])
                        SWIG='echo "Error: Cannot determine SWIG version.  You should look at http://www.swig.org" ; false'
                fi
        fi
        AC_SUBST([SWIG_LIB])
])

# SWIG_ENABLE_CXX()
#
# Enable SWIG C++ support.  This affects all invocations of $(SWIG).
AC_DEFUN([SWIG_ENABLE_CXX],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PROG_CXX])
        SWIG="$SWIG -c++"
])

# SWIG_PYTHON([use-shadow-classes = {no, yes}])
#
# Checks for Python and provides the $(SWIG_PYTHON_CPPFLAGS),
# and $(SWIG_PYTHON_OPT) output variables.
#
# $(SWIG_PYTHON_OPT) contains all necessary SWIG options to generate
# code for Python.  Shadow classes are enabled unless the value of the
# optional first argument is exactly 'no'.  If you need multi module
# support (provided by the SWIG_MULTI_MODULE_SUPPORT() macro) use
# $(SWIG_PYTHON_LIBS) to link against the appropriate library.  It
# contains the SWIG Python runtime library that is needed by the type
# check system for example.
AC_DEFUN([SWIG_PYTHON],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PYTHON_DEVEL])
        test "x$1" != "xno" || swig_shadow=" -noproxy"
        AC_SUBST([SWIG_PYTHON_OPT],[-python$swig_shadow])
        AC_SUBST([SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])
])

AC_DEFUN([AC_PYTHON_DEVEL],[
        #
        # should allow for checking of python version here...
        #
        AC_REQUIRE([AM_PATH_PYTHON])

        # Check for Python include path
        AC_MSG_CHECKING([for Python include path])
        python_path=`echo $PYTHON | sed "s,/bin.*$,,"`
        for i in "$python_path/include/python$PYTHON_VERSION/" "$python_path/include/python/" "$python_path/" ; do
                python_path=`find $i -type f -name Python.h -print | sed "1q"`
                if test -n "$python_path" ; then
                        break
                fi
        done
        python_path=`echo $python_path | sed "s,/Python.h$,,"`
        AC_MSG_RESULT([$python_path])
        if test -z "$python_path" ; then
                AC_MSG_ERROR([cannot find Python include path])
        fi
        AC_SUBST([PYTHON_CPPFLAGS],[-I$python_path])

        # Check for Python library path
        AC_MSG_CHECKING([for Python library path])
        python_path=`echo $PYTHON | sed "s,/bin.*$,,"`
        for i in "$python_path/lib/python$PYTHON_VERSION/config/" "$python_path/lib/python$PYTHON_VERSION/" "$python_path/lib/python/config/" "$python_path/lib/python/" "$python_path/" ; do
                python_path=`find $i -type f -name libpython$PYTHON_VERSION.* -print | sed "1q"`
                if test -n "$python_path" ; then
                        break
                fi
        done
        python_path=`echo $python_path | sed "s,/libpython.*$,,"`
        AC_MSG_RESULT([$python_path])
        if test -z "$python_path" ; then
                AC_MSG_ERROR([cannot find Python library path])
        fi
        AC_SUBST([PYTHON_LDFLAGS],["-L$python_path -lpython$PYTHON_VERSION"])
        #
        python_site=`echo $python_path | sed "s/config/site-packages/"`
        AC_SUBST([PYTHON_SITE_PKG],[$python_site])
        #
        # libraries which must be linked in when embedding
        #
        AC_MSG_CHECKING(python extra libraries)
        PYTHON_EXTRA_LIBS=`$PYTHON -c "import distutils.sysconfig; \
                conf = distutils.sysconfig.get_config_var; \
                print conf('LOCALMODLIBS')+' '+conf('LIBS')"
        AC_MSG_RESULT($PYTHON_EXTRA_LIBS)`
        AC_SUBST(PYTHON_EXTRA_LIBS)
])

### test test test
builtin(include,tcl.m4)

