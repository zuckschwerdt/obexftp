/**
	\file obexftp/cache.h
	ObexFTP client API caching layer.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2002-2007 Christian W. Zuckschwerdt <zany@triq.net>

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

#ifndef OBEXFTP_CACHE_H
#define OBEXFTP_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

void cache_purge(cache_object_t **root, const char *path);

void xfer_purge(obexftp_client_t *cli);

int put_cache_object(obexftp_client_t *cli, /*@only@*/ char *name, /*@only@*/ char *object, int size);

int get_cache_object(const obexftp_client_t *cli, const char *name, char **object, int *size);
	
#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_CACHE_H */

