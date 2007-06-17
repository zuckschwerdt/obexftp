#include <common.h>
#ifdef HAVE_BLUETOOTH
#ifdef HAVE_SDPLIB

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "client.h"

/* Nokia OBEX PC Suite Services */
/* 00005005-0000-1000-8000-0002ee000001 */
/* prefer this over FTP on Series 60 devices */
#define __SVC_UUID_PCSUITE_bytes \
{ 0x00, 0x00, 0x50, 0x05, \
  0x00, 0x00, 0x10, 0x00, 0x80, 0x00, \
  0x00, 0x02, 0xee, 0x00, 0x00, 0x01 }
#define SVC_UUID_PCSUITE ((const uint8_t []) __SVC_UUID_PCSUITE_bytes)

char **obexftp_discover_bt_src(const char *src)
{
	char **res;
  inquiry_info *info = NULL;
  bdaddr_t bdaddr, bdswap;
  char name[248];
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
  DEBUG(1, "%s: Scanning ...\n", __func__);
  flags = IREQ_CACHE_FLUSH; /* only show devices currently in range */
  num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);

  if(num_rsp < 0) 
    {
      perror("Inquiry failed.");
      exit(1);
    }

  dd = hci_open_dev(dev_id); 
  if (dd < 0) 
    {
      perror("HCI device open failed");
      free(info);
      exit(1);
    }
  
  res = calloc(num_rsp + 1, sizeof(char *));
  for(i = 0; i < num_rsp; i++) 
    {
      memset(name, 0, sizeof(name));
      baswap(&bdswap, &(info+i)->bdaddr);

      if(hci_read_remote_name(dd, &(info+i)->bdaddr, sizeof(name), name, 100000) < 0)
	{
          strcpy(name, "No Name");
	}

      DEBUG(1, "%s: Found\t%s\t%s\n", __func__, batostr(&bdswap), name);
      res[i] = strdup(batostr(&bdswap));
  }
  
  close(dd);
  free(info);
  
  return res;
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
  if(sdp_service_search_attr_req(sess, search, SDP_ATTR_REQ_INDIVIDUAL, attrid, &seq) < 0)
    {
      perror("SDP service search");
      sdp_close(sess);
      exit(1);
    }

  sdp_list_free(attrid, 0);
  sdp_list_free(search, 0);

  /* Loop through the list of services */
  for(loop = seq; loop; loop = loop->next)
    {
      sdp_record_t *rec = (sdp_record_t *) loop->data;
      sdp_list_t *access = NULL;

      /* get the RFCOMM channel */
      sdp_get_access_protos(rec, &access);

      if(access)
	{
	  channel = sdp_get_proto_port(access, RFCOMM_UUID);
	}
    }

  sdp_list_free(seq, 0);

  return channel;
}

int obexftp_browse_bt_src(const char *src, const char *addr, int svclass)
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

  if(!sess) 
    {
      perror("Failed to connect to SDP server");
      exit(1);
    }
//  baswap(&bdswap, &bdaddr);
//  *res_bdaddr = batostr(&bdswap);
//  fprintf(stderr, "Browsing %s ...\n", *res_bdaddr);

  /* determine the service class we're looking for */
  if ((svclass != IRMC_SYNC_SVCLASS_ID) &&
      (svclass != OBEX_OBJPUSH_SVCLASS_ID) &&
      (svclass != OBEX_FILETRANS_SVCLASS_ID))
    {
      svclass = OBEX_FILETRANS_SVCLASS_ID;
      /* or OBEX_FILETRANS_PROFILE_ID? */
    }

  /* prefer PCSUITE over FTP */
  if (svclass == OBEX_FILETRANS_SVCLASS_ID)
    {
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

#else
#warning "no bluetooth scan available"
#include "client.h"
char **obexftp_discover_bt()
{
    return NULL;
}
int obexftp_browse_bt(const char *UNUSED(addr), int UNUSED(svclass))
{
    return 0;
}

#endif /* HAVE_SDPLIB */

#else
#warning "no bluetooth discovery available"
#include "client.h"
char **obexftp_discover_bt()
{
    return NULL;
}
int obexftp_browse_bt(const char *UNUSED(addr), int UNUSED(svclass))
{
    return 0;
}

#endif /* HAVE_BLUETOOTH */
