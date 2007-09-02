/**
	\file obexftp/object.h
	Collection of functions to build common OBEX request objects.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>

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

#ifndef OBEXFTP_OBJECT_H
#define OBEXFTP_OBJECT_H

#include <inttypes.h>
#include <openobex/obex.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Telecom/IrMC Synchronization Service: Name Prefix. */
#define IRMC_NAME_PREFIX "telecom/"
/** Telecom/IrMC Synchronization Service: Object Profile file-type. */
#define XOBEX_PROFILE "x-obex/object-profile"
/** Telecom/IrMC Synchronization Service: Capability file-type. */
#define XOBEX_CAPABILITY "x-obex/capability"

/** Folder Browsing Service: Folder Listing file-type. */
#define XOBEX_LISTING "x-obex/folder-listing"

/** Siemens specific: app. param. for memory info.
 * parameter 0x01: mem installed, 0x02: free mem */
#define APPARAM_INFO_CODE '2'


/*@null@*/ obex_object_t *obexftp_build_info (obex_t obex, uint32_t conn, uint8_t opcode);
/*@null@*/ obex_object_t *obexftp_build_get (obex_t obex, uint32_t conn, const char *name, const char *type);
/*@null@*/ obex_object_t *obexftp_build_rename (obex_t obex, uint32_t conn, const char *from, const char *to);
/*@null@*/ obex_object_t *obexftp_build_del (obex_t obex, uint32_t conn, const char *name);
/*@null@*/ obex_object_t *obexftp_build_setpath (obex_t obex, uint32_t conn, const char *name, int create);
/*@null@*/ obex_object_t *obexftp_build_put (obex_t obex, uint32_t conn, const char *name, int size);

#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_OBJECT_H */
