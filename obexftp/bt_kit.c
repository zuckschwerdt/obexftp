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


/**
	Allocate and setup the network stack.

	\note Needed for win32 winsock compatibility.
 */
int btkit_init()
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
int btkit_exit()
{
#ifdef _WIN32
	if (WSACleanup() != 0) {
		DEBUG(2, "%s: WSACleanup failed (%d)\n", __func__, WSAGetLastError());
		return -1;
	}
#endif /* _WIN32 */
	return 0;
}

 
#ifdef HAVE_BLUETOOTH

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


// http://msdn2.microsoft.com/en-us/library/aa362914.aspx
int btkit_browse(const char *src, const char *addr, int svclass)
{
	unsigned int ret;
	WSAQUERYSET querySet;
	char addressAsString[20];
	DWORD addressSize = sizeof(addressAsString);

	memset(&querySet, 0, sizeof(querySet));
	querySet.dwSize = sizeof(querySet);
	querySet.dwNameSpace = NS_BTH;
//	querySet.lpServiceClassId = RFCOMM;
	querySet.lpszContext = addressAsString;

	if (0 == WSAAddressToString(lpSockaddr, sizeof(bdaddr_t), NULL, addressAsString, &addressSize)) {
		printf("search address: %s\n", addressAsString);
	}

	HANDLE hLookup;
	DWORD flags = LUP_NOCONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_ADDR | LUP_RES_SERVICE
	    | LUP_RETURN_NAME | LUP_RETURN_TYPE | LUP_RETURN_BLOB;

	ret = WSALookupServiceBegin(&querySet, flags, &hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceBegin failed (%d)\n", __func__, WSAGetLastError());
	}

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

		DEBUG(3, "%s: Found\t%s\n", __func__, pResults->lpszServiceInstanceName);
	}

	ret = WSALookupServiceEnd(hLookup);
	if (ret != 0) {
		DEBUG(2, "%s: WSALookupServiceEnd failed (%d)\n", __func__, WSAGetLastError());
	}
	return 0;
}

#else /* _WIN32 */

#ifdef HAVE_SDPLIB /* should switch on OS here */


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
// Win32 note:
// WSALookupServiceBegin( WSAQUERYSET with
//  dwNameSpace = NS_BTH
//  lpServiceClassId = RFCOMM
//  lpszContext = WSAAddressToString  )
// WSALookupServiceNext
// WSALookupServiceEnd
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

	/* determine the service class we're looking for */
	if ((svclass != IRMC_SYNC_SVCLASS_ID) &&
			(svclass != OBEX_OBJPUSH_SVCLASS_ID) &&
			(svclass != OBEX_FILETRANS_SVCLASS_ID)) {
		svclass = OBEX_FILETRANS_SVCLASS_ID;
		/* or OBEX_FILETRANS_PROFILE_ID? */
	}

	/* prefer PCSUITE over FTP */
	if (svclass == OBEX_FILETRANS_SVCLASS_ID) {
		sdp_uuid128_create(&root_uuid, &SVC_UUID_PCSUITE);
		res = browse_sdp_uuid(sess, &root_uuid);
		if (res > 0) return res;
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

#endif /* HAVE_SDPLIB */
#endif /* _WIN32 */

#else
#warning "bluetooth not available"
#endif /* HAVE_BLUETOOTH */
