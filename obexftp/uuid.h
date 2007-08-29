/**
	\file obexftp/uuid.h
	Definitions of well-known Universal Unique Identifiers (UUIDs).
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002-2007 Christian W. Zuckschwerdt <zany@triq.net>
 */

#ifndef OBEXFTP_UUID_H
#define OBEXFTP_UUID_H

/**
	Folder Browsing service UUID.
	binary representation of F9EC7BC4-953C-11D2-984E-525400DC9E09
 */
#define __UUID_FBS_bytes \
{ 0xF9, 0xEC, 0x7B, 0xC4, \
  0x95, 0x3C, 0x11, 0xD2, 0x98, 0x4E, \
  0x52, 0x54, 0x00, 0xDC, 0x9E, 0x09 }

#define UUID_FBS ((const uint8_t []) __UUID_FBS_bytes)

/**
	UUID for Siemens S45 and maybe others too.
	binary representation of 6B01CB31-4106-11D4-9A77-0050DA3F471F
 */
#define __UUID_S45_bytes \
{ 0x6B, 0x01, 0xCB, 0x31, \
  0x41, 0x06, 0x11, 0xD4, 0x9A, 0x77, \
  0x00, 0x50, 0xDA, 0x3F, 0x47, 0x1F }

#define UUID_S45 ((const uint8_t []) __UUID_S45_bytes)

/**
	UUID for Telecom/IrMC Synchronization Service (see IrOBEX spec).
	The character string "IRMC-SYNC".
 */
#define __UUID_IRMC_bytes \
{ 'I', 'R', 'M', 'C', '-', 'S', 'Y', 'N', 'C' }

#define UUID_IRMC ((const uint8_t []) __UUID_IRMC_bytes)

/**
	UUID for Sharp mobiles.
	The character string "PCSOFTWARE".
 */
#define __UUID_PCSOFTWARE_bytes \
{ 'P', 'C', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E' }

#define UUID_PCSOFTWARE ((const uint8_t []) __UUID_PCSOFTWARE_bytes)
	
#endif /* OBEXFTP_UUID_H */
