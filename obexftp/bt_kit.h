/**
	\file obexftp/bt_kit.h
	Bluetooth, SDP, HCI kit for Linux, FreeBSD, NetBSD and Win32.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2007 Christian W. Zuckschwerdt <zany@triq.net>

	ObexFTP is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with ObexFTP. If not, see <http://www.gnu.org/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef BTKITSYM
#define BTKITSYM	__attribute__ ((visibility ("hidden")))
#endif

#ifdef HAVE_BLUETOOTH

//#ifndef _WIN32
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
//#endif /* _WIN32 */

/* Windows with headers files from the Platform SDK */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2bth.h>
//#include <Bthsdpdef.h>
//#include <BluetoothAPIs.h>
#define ESOCKTNOSUPPORT	WSAESOCKTNOSUPPORT
#define ETIMEDOUT	WSAETIMEDOUT
#define ECONNREFUSED	WSAECONNREFUSED
#define EHOSTDOWN	WSAEHOSTDOWN
#define EINPROGRESS	WSAEINPROGRESS
#define bdaddr_t	BTH_ADDR
#define sockaddr_rc	_SOCKADDR_BTH
#define rc_family	addressFamily
#define rc_bdaddr	btAddr
#define rc_channel	port
#define PF_BLUETOOTH	PF_BTH
#define AF_BLUETOOTH	PF_BLUETOOTH
#define BTPROTO_RFCOMM	BTHPROTO_RFCOMM
#define BDADDR_ANY	(&(BTH_ADDR){BTH_ADDR_NULL})
#define bacpy(dst,src)	memcpy((dst),(src),sizeof(BTH_ADDR))
#define bacmp(a,b)	memcmp((a),(b),sizeof(BTH_ADDR))
BTKITSYM int ba2str(const bdaddr_t *btaddr, char *straddr);
BTKITSYM int str2ba(const char *straddr, BTH_ADDR *btaddr);

/* FreeBSD 5 and up */
#elif defined(__FreeBSD__)
//#include <sys/types.h>
#include <bluetooth.h>
#define sockaddr_rc	sockaddr_rfcomm
#define rc_family	rfcomm_family
#define rc_bdaddr	rfcomm_bdaddr
#define rc_channel	rfcomm_channel
#define BTPROTO_RFCOMM	BLUETOOTH_PROTO_RFCOMM
#define BDADDR_ANY	(&(bdaddr_t) {{0, 0, 0, 0, 0, 0}})

/* NetBSD-4 and up */
#elif defined(__NetBSD__)
#include <bluetooth.h>
#include <netbt/rfcomm.h>
#define sockaddr_rc	sockaddr_bt
#define rc_family	bt_family
#define rc_bdaddr	bt_bdaddr
#define rc_channel	bt_channel
#define BDADDR_ANY	NG_HCI_BDADDR_ANY

/* BlueZ, Linux 2.4.6 and up (incl. 2.6) */
#else
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#endif /* _WIN32 */

/* library setup/teardown functions (needed for win32) */

BTKITSYM int btkit_init();
BTKITSYM int btkit_exit();

/* additional functions */

BTKITSYM char **btkit_discover(const char *src); /* HCI no. or address */
BTKITSYM char *btkit_getname(const char *src, const char *addr);
BTKITSYM int btkit_browse(const char *src, const char *addr, int svclass);
BTKITSYM int btkit_register_obex(int svclass, int channel);
BTKITSYM int btkit_unregister_service(int svclass);
//KITSYM int btkit_open_rfcomm(char *src, char *dest, int channel);

#endif /* HAVE_BLUETOOTH */
