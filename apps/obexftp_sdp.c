/*
 *
 *  obexftp/obexftp_sdp.c: Transfer from/to Mobile Equipment via OBEX
 *
 *  Copyright (c) 2006 Alan Zhang <vibra@tom.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#ifndef _WIN32
#include <syslog.h>
#define log_err(format, args...) syslog(LOG_ERR, format, ##args)
#define log_err_prefix "ObexFTPd: "
#else
#include <stdio.h>
#define log_err(format, args...) fprintf(stderr, format, ##args)
#define log_err_prefix ""
#endif

#ifdef HAVE_SDPLIB

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "obexftp_sdp.h"

#define OBEXFTP_RFCOMM_CHANNEL	10


static sdp_record_t  *record;
static sdp_session_t *session;


void obexftp_sdp_unregister(void) 
{
	if (record && sdp_record_unregister(session, record))
		log_err("Service record unregistration failed.");

	sdp_close(session);
}

int obexftp_sdp_register(int channel)
{
	sdp_list_t *svclass, *pfseq, *apseq, *root, *aproto;
	uuid_t root_uuid, l2cap, rfcomm, obex, obexftp;
	sdp_profile_desc_t profile[1];
	sdp_list_t *proto[3];
	sdp_data_t *v;
	uint8_t channel_id = OBEXFTP_RFCOMM_CHANNEL;
	int status;

	if (channel > 0) channel_id = channel;
	
	session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, 0);
	if (!session) {
		log_err("Failed to connect to the local SDP server. %s(%d)", 
				strerror(errno), errno);
		return -1;
	}

	record = sdp_record_alloc();
	if (!record) {
		log_err("Failed to allocate service record %s(%d)", 
				strerror(errno), errno);
		sdp_close(session);
		return -1;
	}

	//Register to Public Browse Group
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(record, root);
	sdp_list_free(root, NULL);

	//Protocol Descriptor List: L2CAP
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(NULL, &l2cap);
	apseq    = sdp_list_append(NULL, proto[0]);

	//Protocol Descriptor List: RFCOMM
	sdp_uuid16_create(&rfcomm, RFCOMM_UUID);
	proto[1] = sdp_list_append(NULL, &rfcomm);
	v = sdp_data_alloc(SDP_UINT8, &channel_id);
	proto[1] = sdp_list_append(proto[1], v);
	apseq    = sdp_list_append(apseq, proto[1]);

	//Protocol Descriptor List: OBEX
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

	//Service Class ID List:
	sdp_uuid16_create(&obexftp, OBEX_FILETRANS_SVCLASS_ID);
	svclass = sdp_list_append(NULL, &obexftp);
	sdp_set_service_classes(record, svclass);

	//Profile Descriptor List:
	sdp_uuid16_create(&profile[0].uuid, OBEX_FILETRANS_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(NULL, &profile[0]);
	sdp_set_profile_descs(record, pfseq);
	sdp_set_info_attr(record, "OBEX File Transfer", NULL, NULL);

	status = sdp_device_record_register(session, BDADDR_ANY, record, 0);
	if (status) {
		log_err("SDP registration failed.");
		sdp_record_free(record); record = NULL;
		sdp_close(session);
		return -1;
	}
	return 0;
}

/* Search for OBEX FTP service.
 * Returns 1 if service is found and 0 otherwise. */
/*
int obexftp_sdp_search(bdaddr_t *src, bdaddr_t *dst, uint16_t service)
{
	sdp_list_t *srch, *rsp = NULL;
	sdp_session_t *s;
	uuid_t svclass;
	int err;

	switch (service) {
	case OBEX_FILETRANS_SVCLASS_ID:
		sdp_uuid16_create(&svclass, OBEX_FILETRANS_SVCLASS_ID);
		break;
	default:
		break;
	}
		
	srch = sdp_list_append(NULL, &svclass);

	s = sdp_connect(src, dst, 0);
	if (!s) {
		log_err("Failed to connect to the SDP server. %s(%d)",
				strerror(errno), errno);
		return 0;
	}

	err = sdp_service_search_req(s, srch, 1, &rsp);
	sdp_close(s);

	// Assume that search is successeful
	// if at least one record is found
	if (!err && sdp_list_len(rsp))
		return 1;

	return 0;
}
*/

#else

void obexftp_sdp_unregister(void)
{
}

int obexftp_sdp_register(void)
{
	log_err("SDP not supported.");
	return -1;
}

#endif // HAVE_SDPLIB
