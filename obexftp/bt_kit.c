/**
	\file obexftp/bt_kit.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2bth.h>
#define WSA_VER_MAJOR 2
#define WSA_VER_MINOR 2
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif /* _WIN32 */

#include "bt_kit.h"

#include <common.h>


#ifdef HAVE_BLUETOOTH

/**
	Nokia OBEX PC Suite Services.
	binary representation of 00005005-0000-1000-8000-0002ee000001
	\note prefer this over FTP on Series 60 devices
 */
#define __SVC_UUID_PCSUITE_bytes \
{ 0x00, 0x00, 0x50, 0x05, \
  0x00, 0x00, 0x10, 0x00, 0x80, 0x00, \
  0x00, 0x02, 0xee, 0x00, 0x00, 0x01 }
#define SVC_UUID_PCSUITE ((const uint8_t []) __SVC_UUID_PCSUITE_bytes)

//Nokia SyncML Server UUID 128: 00005601-0000-1000-8000-0002ee000001

/* well known services 0x1000 to 0x12FF */
#define __SVC_UUID_BASE_bytes \
{ 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x10, 0x00, 0x80, 0x00, \
  0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } 
#define SVC_UUID_BASE ((const uint8_t []) __SVC_UUID_BASE_bytes)

/* 0x0001: server; 0x0002: client; 0x0003: DM server; 0x0004: DM client */
#define __SVC_UUID_SYNCML_bytes \
{ 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x10, 0x00, 0x80, 0x00, \
  0x00, 0x02, 0xEE, 0x00, 0x00, 0x02 } 
#define SVC_UUID_SYNCML ((const uint8_t []) __SVC_UUID_SYNCML_bytes)


/**
	Allocate and setup the network stack.

	\note Needed for win32 winsock compatibility.
 */
int btkit_init(void)
{
#ifdef _WIN32
	WORD wVersionRequired = MAKEWORD(WSA_VER_MAJOR,WSA_VER_MINOR);
	WSADATA lpWSAData;
	if (WSAStartup(wVersionRequired, &lpWSAData) != 0) {
		DEBUG(2, "%s: WSAStartup failed (%d)\n", __func__, WSAGetLastError());
		return -1;
	}
	if (LOBYTE(lpWSAData.wVersion) != WSA_VER_MAJOR ||
	    HIBYTE(lpWSAData.wVersion) != WSA_VER_MINOR) {
		DEBUG(2, "%s: WSA version mismatch\n", __func__);
		WSACleanup();
		return -1;
	}
#endif /* _WIN32 */
	return 0;
}


/**
	Tear-down and free the network stack.

	\note Needed for win32 winsock compatibility.
 */
int btkit_exit(void)
{
#ifdef _WIN32
	if (WSACleanup() != 0) {
		DEBUG(2, "%s: WSACleanup failed (%d)\n", __func__, WSAGetLastError());
		return -1;
	}
#endif /* _WIN32 */
	return 0;
}

 
#ifdef _WIN32
//void baswap(bdaddr_t *dst, const bdaddr_t *src)
//bdaddr_t *strtoba(const char *str)
//char *batostr(const bdaddr_t *ba)

/**
	Implementation of ba2str for winsock2.
 */
int ba2str(const bdaddr_t *btaddr, char *straddr)
{
	/* WSAAddressToString() is not useful, it adds parentheses */
        unsigned char *b = btaddr;
        return sprintf(straddr, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
                b[7], b[6], b[5], b[4], b[3], b[2]);

}


/**
	Implementation of str2ba for winsock2.
 */
int str2ba(const char *straddr, bdaddr_t *btaddr)
{
	int i;
	unsigned int aaddr[6];
	bdaddr_t tmpaddr = 0;

	if (sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x",
			&aaddr[0], &aaddr[1], &aaddr[2],
			&aaddr[3], &aaddr[4], &aaddr[5]) != 6)
		return 1;
	*btaddr = 0;
	for (i = 0; i < 6; i++) {
		tmpaddr = (bdaddr_t) (aaddr[i] & 0xff);
		*btaddr = ((*btaddr) << 8) + tmpaddr;
	}
	return 0;
}


char **btkit_discover(const char *src)
{
	unsigned int ret;
	WSAQUERYSET querySet;
	char addressAsString[18];
	char **res, **p;

	memset(&querySet, 0, sizeof(querySet));
	querySet.dwSize = sizeof(querySet);
	querySet.dwNameSpace = NS_BTH;

	HANDLE hLookup;
	DWORD flags = LUP_CONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_ADDR | LUP_RES_SERVICE;

	ret = WSALookupServiceBegin(&querySet, flags, &hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceBegin failed (%d)\n", __func__, WSAGetLastError());
	}

	p = res = calloc(15 + 1, sizeof(char *));
	for (;;) {

		BYTE buffer[1000];
		DWORD bufferLength = sizeof(buffer);
		WSAQUERYSET *pResults = (WSAQUERYSET*)&buffer;

		ret = WSALookupServiceNext(hLookup, flags, &bufferLength, pResults);
		if (GetLastError() == WSA_E_NO_MORE) {
			break;
		}
		if (ret != 0) {
			DEBUG(2, "%s: WSALookupServiceNext failed (%d)\n", __func__, WSAGetLastError());
		}

		ba2str(pResults->lpcsaBuffer->RemoteAddr.lpSockaddr, addressAsString);
		DEBUG(3, "%s: Found\t%s\n", __func__, addressAsString);
		*p++ = strdup(addressAsString);
	}

	ret = WSALookupServiceEnd(hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceEnd failed (%d)\n", __func__, WSAGetLastError());
	}
	return res;
}


char *btkit_getname(const char *src, const char *addr)
{
	unsigned int ret;
	WSAQUERYSET querySet;
	char addressAsString[18];
	char *res = NULL;

	memset(&querySet, 0, sizeof(querySet));
	querySet.dwSize = sizeof(querySet);
	querySet.dwNameSpace = NS_BTH;

	HANDLE hLookup;
	DWORD flags = LUP_CONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_ADDR | LUP_RES_SERVICE;

	ret = WSALookupServiceBegin(&querySet, flags, &hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceBegin failed (%d)\n", __func__, WSAGetLastError());
	}

	for (;;) {

		BYTE buffer[1000];
		DWORD bufferLength = sizeof(buffer);
		WSAQUERYSET *pResults = (WSAQUERYSET*)&buffer;

		ret = WSALookupServiceNext(hLookup, flags | LUP_RETURN_NAME, &bufferLength, pResults);
		if (GetLastError() == WSA_E_NO_MORE) {
			break;
		}
		if (ret != 0) {
			DEBUG(2, "%s: WSALookupServiceNext failed (%d)\n", __func__, WSAGetLastError());
		}

		ba2str(pResults->lpcsaBuffer->RemoteAddr.lpSockaddr, addressAsString);
		if (!strcmp(addressAsString, addr)) {
			DEBUG(3, "%s: Found\t%s\n", __func__, pResults->lpszServiceInstanceName);
			res = pResults->lpszServiceInstanceName;
			break;
		}
		DEBUG(3, "%s: Skipping\t%s\n", __func__, pResults->lpszServiceInstanceName);
	}

	ret = WSALookupServiceEnd(hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceEnd failed (%d)\n", __func__, WSAGetLastError());
	}
	return res;
}


int btkit_browse(const char *src, const char *addr, int svclass)
{
	unsigned int ret;
	int port = 0;
	WSAQUERYSET querySet;
	char addressAsString[20]; // "(XX:XX:XX:XX:XX:XX)"
	snprintf(addressAsString, 20, "(%s)", addr);

	if (!addr || strlen(addr) != 17) {
		DEBUG(1, "%s: bad address\n", __func__);
		return -1;
	}

	if ((svclass<0x0001 || svclass>0x0004)
	 && (svclass<0x1000 || svclass>0x12FF)) {
		DEBUG(1, "%s: bad service class\n", __func__);
		return -1;
	}

	GUID baseServiceClassId = {
		0x00000000,
		0x0000, 
		0x1000, 
		{ 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } 
	}; // Bluetooth_Base_UUID
	baseServiceClassId.Data1 = svclass;
	
	GUID syncmlClassId = {
		0x00000000,
		0x0000, 
		0x1000, 
		{ 0x80, 0x00, 0x00, 0x02, 0xEE, 0x00, 0x00, 0x02 } 
	}; // common UUID for SyncML Client/Server
	syncmlClassId.Data1 = svclass;

	memset(&querySet, 0, sizeof(querySet));
	querySet.dwSize = sizeof(querySet);
	querySet.dwNameSpace = NS_BTH;
	if (svclass>=0x0001 && svclass<=0x0004)
		querySet.lpServiceClassId = &syncmlClassId;
	else
		querySet.lpServiceClassId = &baseServiceClassId;
	querySet.lpszContext = addressAsString;

	HANDLE hLookup;
	DWORD flags = LUP_NOCONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_ADDR | LUP_RES_SERVICE
	    | LUP_RETURN_NAME;

	ret = WSALookupServiceBegin(&querySet, flags, &hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceBegin failed (%d)\n", __func__, WSAGetLastError());
		return -1;
	}

	for (;;) {

		BYTE buffer[4000];
		DWORD bufferLength = sizeof(buffer);
		WSAQUERYSET *pResults = (WSAQUERYSET*)&buffer;

		ret = WSALookupServiceNext(hLookup, flags, &bufferLength, pResults);
		if (GetLastError() == WSA_E_NO_MORE) {
			break;
		}
		if (ret != 0) {
			DEBUG(2, "%s: WSALookupServiceNext failed (%d)\n", __func__, WSAGetLastError());
			break;
		}

		port = ((SOCKADDR_BTH*)pResults->lpcsaBuffer->RemoteAddr.lpSockaddr)->port;
		DEBUG(3, "%s: Found\t%s\t%li\n", __func__, pResults->lpszServiceInstanceName, port);
	}

	ret = WSALookupServiceEnd(hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceEnd failed (%d)\n", __func__, WSAGetLastError());
		return -1;
	}
	return port;
}


int btkit_register_obex(int UNUSED(svclass), int UNUSED(channel))
{
	DEBUG(1, "Implement this stub.");
	return -1;
}


int btkit_unregister_service(int UNUSED(svclass))
{
	DEBUG(1, "Implement this stub.");
	return -1;
}

#else /* _WIN32 */

#ifdef HAVE_SDP
#if defined(__NetBSD__) || defined(__FreeBSD__)

char **btkit_discover(const char *src)
{
	struct bt_devinquiry *di;
	char **res;
	int num_rsp = 10;
	int length = 8;
	int i;

	DEBUG(2, "%s: Scanning ...\n", __func__);

	num_rsp = bt_devinquiry(src, length, num_rsp, &di);
	if(num_rsp < 0) {
		DEBUG(1, "%s: Inquiry failed\n", __func__);
		return NULL;
	}

	res = calloc(num_rsp + 1, sizeof(char *));
	for(i = 0; i < num_rsp; i++) {
		res[i] = bt_malloc(18);	/* size of text bdaddr */
		bt_ntoa(&di->bdaddr, res[i]);
		DEBUG(2, "%s: Found\t%s\n", __func__, res[i]);
	}

	free(di);

	return res;
}

static int btkit_getname_cb(int UNUSED(s), const struct bt_devinfo *di, void *arg)
{

	if ((di->enabled)) {
		strncpy(arg, di->devname, HCI_DEVNAME_SIZE);
		return 1;
	}

	return 0;
}

char *btkit_getname(const char *src, const char *addr)
{
	hci_remote_name_req_cp cp;
	hci_remote_name_req_compl_ep ep;
	struct bt_devreq req;
	int s;

	return_val_if_fail(addr != NULL, NULL);

	if (!src) {
		if (bt_devenum(btkit_getname_cb, ep.name) == -1) {
			DEBUG(1, "%s: device enumeration failed\n", __func__);
			return NULL;
		}

		src = (const char *)ep.name;
	}

	s = bt_devopen(src, 0);
	if (s == -1) {
		DEBUG(1, "%s: HCI device open failed\n", __func__);
		return NULL;
	}

	memset(&cp, 0, sizeof(cp));
	bt_aton(addr, &cp.bdaddr);

	memset(&req, 0, sizeof(req));
	req.opcode = HCI_CMD_REMOTE_NAME_REQ;
	req.cparam = &cp;
	req.clen = sizeof(cp);
	req.event = HCI_EVENT_REMOTE_NAME_REQ_COMPL;
	req.rparam = &ep;
	req.rlen = sizeof(ep);

	if (bt_devreq(s, &req, 100) == -1) {
		DEBUG(1, "%s: remote name request failed\n", __func__);
		strcpy(ep.name, "No Name");
	}

	close(s);

	return strndup(ep.name, sizeof(ep.name));
}

static int btkit_browse_sdp(sdp_session_t ss, uuid_t *uuid)
{
	uint8_t buf[19];	/* enough for uuid128 (ssp) and uint16 (ail) */
	sdp_data_t seq, ssp, ail, rsp, rec, value, pdl;
	uintmax_t channel;
	uint16_t attr;

	seq.next = buf;
	seq.end = buf + sizeof(buf);

	/* build ServiceSearchPattern */
	ssp.next = seq.next;
	sdp_put_uuid(&seq, uuid);
	ssp.end = seq.next;

	/* build AttributeIDList */
	ail.next = seq.next;
	sdp_put_uint16(&seq, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	ail.end = seq.next;

	if (!sdp_service_search_attribute(ss, &ssp, &ail, &rsp)) {
		DEBUG(1, "%s: SDP service search failed\n", __func__);
		return -1;
	}

	/*
	 * we expect the response to contain a list of records
	 * containing ProtocolDescriptorList. Return the first
	 * one with a valid RFCOMM channel.
	 */
	while (sdp_get_seq(&rsp, &rec)) {
		if (!sdp_get_attr(&rec, &attr, &value)
		    || attr != SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST)
			continue;

		/* drop any alt header */
		sdp_get_alt(&value, &value);

		/* for each protocol stack */
		while (sdp_get_seq(&value, &pdl)) {
			/* and for each protocol */
			while (sdp_get_seq(&pdl, &seq)) {
				/* check for RFCOMM */
				if (sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_RFCOMM)
				    && sdp_get_uint(&seq, &channel)
				    && channel >= RFCOMM_CHANNEL_MIN
				    && channel <= RFCOMM_CHANNEL_MAX)
					return channel;
			}
		}
	}

	DEBUG(1, "%s: no channel found\n", __func__);

	return -1;
}

int btkit_browse(const char *src, const char *addr, int svclass)
{
	bdaddr_t laddr, raddr;
	sdp_session_t ss;
	uuid_t uuid;
	int channel;

	return_val_if_fail(addr != NULL, -1);
	bt_aton(addr, &raddr);

	if (src) {
		if (!bt_devaddr(src, &laddr)) {
			DEBUG(1, "%s: invalid source address\n", __func__);
			return -1;
		}
	} else {
		bdaddr_copy(&laddr, BDADDR_ANY);
	}

	ss = sdp_open(&laddr, &raddr);
	if (ss == NULL) {
		DEBUG(1, "%s: Failed to connect to SDP server\n", __func__);
		return -1;
	}

	if (svclass >= 0x0001 && svclass <= 0x0004) {
		uuid_dec_be(SVC_UUID_SYNCML, &uuid);
		uuid.time_low = svclass;
	} else if (svclass >= 0x1000 && svclass <= 0x12FF) {
		uuid_dec_be(SVC_UUID_BASE, &uuid);
		uuid.time_low = svclass;
	} else {
		svclass = SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER;
	}

	DEBUG(1, "%s: svclass 0x%04x\n", __func__, svclass);

	channel = -1;

	/* Try PCSUITE first */
	if (svclass == SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER) {
		DEBUG(1, "%s: trying PCSUITE first\n", __func__);
		uuid_dec_be(SVC_UUID_PCSUITE, &uuid);
		channel = btkit_browse_sdp(ss, &uuid);

		uuid_dec_be(SVC_UUID_BASE, &uuid);
		uuid.time_low = svclass;
	}

	if (channel == -1)
		channel = btkit_browse_sdp(ss, &uuid);

	sdp_close(ss);
	return channel;
}

static sdp_session_t ss;
static uint32_t opush_handle;
static uint32_t ftrn_handle;
static uint32_t irmc_handle;

int btkit_register_obex(int svclass, int channel)
{
	uint8_t buffer[256];
	sdp_data_t rec;
	uint32_t *hp;

	DEBUG(1, "%s: svclass 0x%04x channel %d\n", __func__, svclass, channel);

	/* Build SDP record */
	rec.next = buffer;
	rec.end = buffer + sizeof(buffer);

	sdp_put_uint16(&rec, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(&rec, 0x00000000);

	sdp_put_uint16(&rec, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(&rec, 3);
	sdp_put_uuid16(&rec, (uint16_t)svclass);

	sdp_put_uint16(&rec, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(&rec, 17);
	sdp_put_seq(&rec, 3);
	sdp_put_uuid16(&rec, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_seq(&rec, 5);
	sdp_put_uuid16(&rec, SDP_UUID_PROTOCOL_RFCOMM);
	sdp_put_uint8(&rec, (uint8_t)channel);
	sdp_put_seq(&rec, 3);
	sdp_put_uuid16(&rec, SDP_UUID_PROTOCOL_OBEX);

	sdp_put_uint16(&rec, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(&rec, 3);
	sdp_put_uuid16(&rec, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(&rec, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(&rec, 9);
	sdp_put_uint16(&rec, 0x656e);	/* "en" */
	sdp_put_uint16(&rec, 106);	/* UTF-8 */
	sdp_put_uint16(&rec, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(&rec, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(&rec, 8);
	sdp_put_seq(&rec, 6);
	sdp_put_uuid16(&rec, (uint16_t)svclass);
	sdp_put_uint16(&rec, 0x0100);	/* v1.0 */

	sdp_put_uint16(&rec, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID + SDP_ATTR_SERVICE_NAME_OFFSET);
	switch (svclass) {
	case SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH:
		sdp_put_str(&rec, "OBEX Object Push", -1);

		sdp_put_uint16(&rec, SDP_ATTR_SUPPORTED_FORMATS_LIST);
		sdp_put_seq(&rec, 2);
		sdp_put_uint8(&rec, 0xff);	/* Any */
		hp = &opush_handle;
		break;

	case SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER:
		sdp_put_str(&rec, "OBEX File Transfer", -1);
		hp = &ftrn_handle;
		break;

	case SDP_SERVICE_CLASS_IR_MC_SYNC:
		sdp_put_str(&rec, "IrMC Sync", -1);
		hp = &irmc_handle;
		break;

	default:
		DEBUG(1, "%s: unknown svclass\n", __func__);
		return -1;
		break;
	}

	rec.end = rec.next;
	rec.next = buffer;

	/* Register service with SDP server */
	if (ss == NULL) {
		ss = sdp_open_local(NULL);
		if (ss == NULL) {
			DEBUG(1, "%s: failed to open SDP session\n", __func__);
			return -1;
		}
		DEBUG(2, "%s: opened SDP session\n", __func__);
	}

	if (!sdp_record_insert(ss, NULL, hp, &rec)) {
		DEBUG(1, "%s: failed to insert SDP record\n", __func__);
		sdp_data_print(&rec, 2);
		return -1;
	}

	return 0;
}

int btkit_unregister_service(int svclass)
{
	uint32_t *hp;

	if (ss == NULL) {
		DEBUG(1, "%s: no session open\n", __func__);
		return -1;
	}

	switch (svclass) {
	case SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH:
		hp = &opush_handle;
		break;

	case SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER:
		hp = &ftrn_handle;
		break;

	case SDP_SERVICE_CLASS_IR_MC_SYNC:
		hp = &irmc_handle;
		break;

	default:
		DEBUG(1, "%s: unknown svclass\n", __func__);
		return -1;
		break;
	}

	if (!sdp_record_remove(ss, *hp)) {
		DEBUG(1, "%s: failed to remove SDP record\n", __func__);
		return -1;
	}

	*hp = 0;

	if (opush_handle == 0 && ftrn_handle == 0 && irmc_handle == 0) {
		DEBUG(2, "%s: closed SDP session\n", __func__);
		sdp_close(ss);
		ss = NULL;
	}

	return 0;
}

#else /* defined(__NetBSD__) || defined(__FreeBSD__) */

/**
	Discover all bluetooth devices in range.
	\param src optional source interface address (HCI or MAC)
	\return an array of device addresses
 */
char **btkit_discover(const char *src)
{
	char **res;
	inquiry_info *info = NULL;
	bdaddr_t bdswap;
	int dev_id;
	int num_rsp = 10;
	int flags = 0;
	int length = 8;
	int dd, i;

	/* Get local bluetooth address */
	if (src && strlen(src) == 17)
		 dev_id = hci_devid(src);
	else if (src)
		dev_id = atoi(src);
	else
		dev_id = hci_get_route(NULL);
	DEBUG(2, "%s: Scanning ...\n", __func__);
	flags = IREQ_CACHE_FLUSH; /* only show devices currently in range */
	num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);

	if(num_rsp < 0) {
		DEBUG(1, "%s: Inquiry failed", __func__);
		return NULL;
	}

	dd = hci_open_dev(dev_id); 
	if (dd < 0) {
		DEBUG(1, "%s: HCI device open failed", __func__);
		free(info);
		return NULL;
	}
  
	res = calloc(num_rsp + 1, sizeof(char *));
	for(i = 0; i < num_rsp; i++) {
		baswap(&bdswap, &(info+i)->bdaddr);
		res[i] = batostr(&bdswap);
		DEBUG(2, "%s: Found\t%s\n", __func__, res[i]);
	}
  
	hci_close_dev(dd);
	free(info);
  
	return res;
}


/**
	Get the name of a bluetooth device.
	\param src optional source interface address (HCI or MAC)
	\param addr the bluetooth address of the device to query
	\return the bluetooth name of the device
 */
char *btkit_getname(const char *src, const char *addr)
{
	bdaddr_t bdaddr;
	int dev_id, dd;
	char name[248];

	return_val_if_fail(addr != NULL, NULL);
	str2ba(addr, &bdaddr);

	/* Get local bluetooth address */
	if (src && strlen(src) == 17)
		dev_id = hci_devid(src);
	else if (src)
		dev_id = atoi(src);
	else
		dev_id = hci_get_route(NULL);

	dd = hci_open_dev(dev_id); 
	if (dd < 0) {
		DEBUG(1, "%s: HCI device open failed", __func__);
		return NULL;
	}

	if(hci_read_remote_name(dd, &bdaddr, sizeof(name), name, 100000) < 0) {
		strcpy(name, "No Name");
	}
	hci_close_dev(dd);

	return strdup(name);
}


static int browse_sdp_uuid(sdp_session_t *sess, uuid_t *uuid)
{
	sdp_list_t *attrid, *search, *seq, *loop;
	uint32_t range = SDP_ATTR_PROTO_DESC_LIST;
	/* 0x0000ffff for SDP_ATTR_REQ_RANGE */
	int channel = -1;

	attrid = sdp_list_append(0, &range);
	search = sdp_list_append(0, uuid);

	/* Get a linked list of services */
	if(sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_INDIVIDUAL, attrid, &seq) < 0) {
		DEBUG(1, "%s: SDP service search failed", __func__);
		sdp_close(sess);
		return -1;
	}

	sdp_list_free(attrid, 0);
	sdp_list_free(search, 0);

	/* Loop through the list of services */
	for(loop = seq; loop; loop = loop->next) {
		sdp_record_t *rec = (sdp_record_t *) loop->data;
		sdp_list_t *access = NULL;

		/* get the RFCOMM channel */
		sdp_get_access_protos(rec, &access);

		if(access) {
			channel = sdp_get_proto_port(access, RFCOMM_UUID);
		}
	}

	sdp_list_free(seq, 0);

	return channel;
}


/**
	Browse a bluetooth device for a given service class.
	\param src optional source interface address (HCI or MAC)
	\param addr the bluetooth address of the device to query
	\return the channel on which the service runs
 */
int btkit_browse(const char *src, const char *addr, int svclass)
{
	int res = -1;
	int dev_id;
	sdp_session_t *sess;
	uuid_t root_uuid;
	bdaddr_t bdaddr;

	if (!addr || strlen(addr) != 17)
		return -1;
	str2ba(addr, &bdaddr);

	/* Get local bluetooth address */
	if (src && strlen(src) == 17)
		dev_id = hci_devid(src);
	else if (src)
		dev_id = atoi(src);
	else
		dev_id = hci_get_route(NULL);

	/* Connect to remote SDP server */
	sess = sdp_connect(BDADDR_ANY, &bdaddr, SDP_RETRY_IF_BUSY);

	if(!sess) {
		DEBUG(1, "%s: Failed to connect to SDP server", __func__);
		return -1;
	}
//	baswap(&bdswap, &bdaddr);
//	*res_bdaddr = batostr(&bdswap);
//	fprintf(stderr, "Browsing %s ...\n", *res_bdaddr);

	/* special case: SyncML */
	if (svclass >= 0x0001 && svclass <= 0x0004) {
		unsigned short data1 = svclass;
		sdp_uuid128_create(&root_uuid, &SVC_UUID_SYNCML);
		memcpy(&root_uuid.value.uuid128.data[2], &data1, 2);
		res = browse_sdp_uuid(sess, &root_uuid);
		sdp_close(sess);
		return res;	
	}

	/* determine the service class we're looking for */
	if (svclass < 0x1000 || svclass > 0x12FF) {
		svclass = OBEX_FILETRANS_SVCLASS_ID;
	}

	/* prefer PCSUITE over FTP */
	if (svclass == OBEX_FILETRANS_SVCLASS_ID) {
		sdp_uuid128_create(&root_uuid, &SVC_UUID_PCSUITE);
		res = browse_sdp_uuid(sess, &root_uuid);
		if (res > 0) {
			sdp_close(sess);
			return res;	
		}
	}

	/* browse for the service class */
	sdp_uuid16_create(&root_uuid, svclass);
	res = browse_sdp_uuid(sess, &root_uuid);

	sdp_close(sess);

	return res;
}


/**
	Search for OBEX FTP service.
	\return 1 if service is found and 0 otherwise
 */
static sdp_record_t *sdp_search_service(sdp_session_t *sess, uint16_t service)
{
	sdp_list_t *attrid, *srch, *rsp = NULL;
	uint32_t range = 0x0000ffff;
	uuid_t svclass;
	int ret;

	attrid = sdp_list_append(0, &range);
       	sdp_uuid16_create(&svclass, service);
	srch = sdp_list_append(NULL, &svclass);

	ret = sdp_service_search_attr_req(sess, srch, SDP_ATTR_REQ_RANGE, attrid, &rsp);
	sdp_list_free(attrid, 0);
	sdp_list_free(srch, 0);
	if (ret < 0) {
		DEBUG(1, "Failed to search the local SDP server."); 
		return NULL;
	}

	if (sdp_list_len(rsp) == 0) {
		DEBUG(1, "No records found on the local SDP server.");
		return NULL;
	}
	return (sdp_record_t *) rsp->data;
}


/**
	Delete a service record from the local SDP server.
 */
int btkit_unregister_service(int svclass) 
{
	sdp_session_t *session;
	sdp_record_t  *record;

	session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, 0);
	if (!session) {
		DEBUG(1, "Failed to connect to the local SDP server.");
		return -1;
	}
	record = sdp_search_service(session, svclass);

	if (record && sdp_record_unregister(session, record))
		DEBUG(1, "Service record unregistration failed.");

	sdp_close(session);

	return 0;
}


/**
	Add a service record to the local SDP server.
 */
int btkit_register_obex(int service, int channel)
{
	sdp_session_t *session;
	sdp_record_t  *record;
	sdp_list_t *svclass, *pfseq, *apseq, *root, *aproto;
	uuid_t root_uuid, l2cap, rfcomm, obex, obexftp;
	sdp_profile_desc_t profile;
	sdp_list_t *proto[3];
	sdp_data_t *v;
	uint8_t channel_id = 1; /* should look for a free one */
	int status;

	if (channel > 0) channel_id = channel;
	
	session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, 0);
	if (!session) {
		DEBUG(1, "Failed to connect to the local SDP server.");
		return -1;
	}

	record = sdp_record_alloc();
	if (!record) {
		DEBUG(1, "Failed to allocate service record.");
		sdp_close(session);
		return -1;
	}

	/* Register to Public Browse Group */
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(record, root);
	sdp_list_free(root, NULL);

	/* Protocol Descriptor List: L2CAP */
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(NULL, &l2cap);
	apseq    = sdp_list_append(NULL, proto[0]);

	/* Protocol Descriptor List: RFCOMM */
	sdp_uuid16_create(&rfcomm, RFCOMM_UUID);
	proto[1] = sdp_list_append(NULL, &rfcomm);
	v = sdp_data_alloc(SDP_UINT8, &channel_id);
	proto[1] = sdp_list_append(proto[1], v);
	apseq    = sdp_list_append(apseq, proto[1]);

	/* Protocol Descriptor List: OBEX */
	sdp_uuid16_create(&obex, OBEX_UUID);
	proto[2] = sdp_list_append(NULL, &obex);
	apseq    = sdp_list_append(apseq, proto[2]);
	
	aproto   = sdp_list_append(NULL, apseq);
	sdp_set_access_protos(record, aproto);
	
	sdp_list_free(proto[0], NULL);
	sdp_list_free(proto[1], NULL);
	sdp_list_free(proto[2], NULL);
	sdp_list_free(apseq, NULL);
	sdp_list_free(aproto, NULL);
	sdp_data_free(v);

	/* Service Class ID List: */
	sdp_uuid16_create(&obexftp, service);
	svclass = sdp_list_append(NULL, &obexftp);
	sdp_set_service_classes(record, svclass);

	/* Profile Descriptor List: */
	/* profile id matches service id here */
	sdp_uuid16_create(&profile.uuid, service);
	profile.version = 0x0100;
	pfseq = sdp_list_append(NULL, &profile);
	sdp_set_profile_descs(record, pfseq);
	sdp_set_info_attr(record, "OBEX File Transfer", NULL, NULL);

	status = sdp_device_record_register(session, BDADDR_ANY, record, SDP_RECORD_PERSIST);
	if (status < 0) {
		DEBUG(1, "SDP registration failed.");
		sdp_record_free(record); record = NULL;
		sdp_close(session);
		return -1;
	}

	sdp_close(session);
	return 0;
}

#endif	/* BlueZ/Linux */

#else
#warning "no bluetooth sdp support for this platform"

char **btkit_discover(const char *UNUSED(src))
{
	return NULL;
}
char *btkit_getname(const char *UNUSED(src), const char *UNUSED(addr))
{
	return NULL;
}
int btkit_browse(const char *UNUSED(src), const char *UNUSED(addr), int UNUSED(svclass))
{
	return -1;
}

int btkit_register_obex(int UNUSED(svclass), int UNUSED(channel))
{
	DEBUG(1, "SDP not supported.");
	return -1;
}

int btkit_unregister_service(int UNUSED(svclass))
{
	return -1;
}

#endif /* HAVE_SDP */
#endif /* _WIN32 */

#else
#warning "bluetooth not available"
#endif /* HAVE_BLUETOOTH */
