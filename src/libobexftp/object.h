#ifndef OBEXFTP_OBJECT_H
#define OBEXFTP_OBJECT_H

#include <openobex/obex.h>

/* Telecom/IrMC Synchronization Service */
#define IRMC_NAME_PREFIX "telecom/"
#define XOBEX_PROFILE "x-obex/object-profile"
#define XOBEX_CAPABILITY "x-obex/capability"

/* Folder Browsing Service */
#define XOBEX_LISTING "x-obex/folder-listing"

/* Siemens specific */
/* parameter 0x01: mem installed, 0x02: free mem */
#define APPARAM_INFO_CODE '2'


obex_object_t *obexftp_build_info (obex_t obex, guint8 opcode);
obex_object_t *obexftp_build_list (obex_t obex, const gchar *name);
obex_object_t *obexftp_build_get (obex_t obex, const gchar *name);
obex_object_t *obexftp_build_rename (obex_t obex, const gchar *from, const gchar *to);
obex_object_t *obexftp_build_del (obex_t obex, const gchar *name);
obex_object_t *obexftp_build_setpath (obex_t obex, const gchar *name, gboolean up);
obex_object_t *obexftp_build_put (obex_t obex, const gchar *name);

#endif // OBEXFTP_OBJECT_H
