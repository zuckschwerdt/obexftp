/**
	\file obexftp/unicode.h
	Unicode charset and encoding conversions.
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

#ifndef OBEXFTP_UNICODE_H
#define OBEXFTP_UNICODE_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

int CharToUnicode(uint8_t *uc, const uint8_t *c, int size);
int UnicodeToChar(uint8_t *c, const uint8_t *uc, int size);
int Utf8ToChar(uint8_t *c, const uint8_t *uc, int size);

#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_UNICODE_H */
