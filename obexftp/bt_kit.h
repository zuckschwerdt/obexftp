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

#ifndef BT_KIT_H
#define BT_KIT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef BTKITSYM
#define BTKITSYM	__attribute__ ((visibility ("hidden")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_BLUETOOTH

/* Service Class UUIDs for bt browse. */
/* Only Service Class UUID-16s are accepted (0x1000-0x12FF). Esp. not Protocol UUIDs. */
/* Also SYNCML UUID-16s (0x0001-0x0004) are translated to correct UUID-128s. */
#define BTKIT_SPP_SERVICE	(0x1101)	/* aka SerialPortServiceClassID_UUID16 */
#define BTKIT_SYNC_SERVICE	(0x1104)	/* aka IrMCSyncServiceClassID_UUID16 */
#define BTKIT_PUSH_SERVICE	(0x1105)	/* aka OBEXObjectPushServiceClassID_UUID16 */
#define BTKIT_FTP_SERVICE	(0x1106)	/* aka OBEXFileTransferServiceClassID_UUID16 */
#define BTKIT_SYNCML_SERVER	(0x0001)	/* aka SyncMLServer_UUID */
#define BTKIT_SYNCML_CLIENT	(0x0002)	/* aka SyncMLClient_UUID */
#define BTKIT_SYNCML_DM_SERVER	(0x0003)	/* aka SyncMLDMServer_UUID */
#define BTKIT_SYNCML_DM_CLIENT	(0x0004)	/* aka SyncMLDMClient_UUID */

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

/* Various BSD systems */
#elif defined(__NetBSD__) || defined(__FreeBSD__)
#define COMPAT_BLUEZ
#include <bluetooth.h>
#ifdef HAVE_SDP
#include <sdp.h>
#include <string.h>
#endif

#ifndef BDADDR_ANY
#define BDADDR_ANY	NG_HCI_BDADDR_ANY
#endif

#ifndef RFCOMM_CHANNEL_MIN
#define RFCOMM_CHANNEL_MIN	1
#endif

#ifndef RFCOMM_CHANNEL_MAX
#define RFCOMM_CHANNEL_MAX	30
#endif

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

BTKITSYM int btkit_init(void);
BTKITSYM int btkit_exit(void);

/* additional functions */

BTKITSYM char **btkit_discover(const char *src); /* HCI no. or address */
BTKITSYM char *btkit_getname(const char *src, const char *addr);
BTKITSYM int btkit_browse(const char *src, const char *addr, int svclass);
BTKITSYM int btkit_register_obex(int svclass, int channel);
BTKITSYM int btkit_unregister_service(int svclass);
//KITSYM int btkit_open_rfcomm(char *src, char *dest, int channel);

#endif /* HAVE_BLUETOOTH */

#ifdef __cplusplus
}
#endif

#endif /* BT_KIT_H */
