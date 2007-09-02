/**
	\file obexftp/obexftp_io.h
	ObexFTP IO abstraction.
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

#ifndef OBEXFTP_IO_H
#define OBEXFTP_IO_H

/*@null@*/ obex_object_t *build_object_from_file(obex_t *handle, uint32_t conn, const char *localname, const char *remotename);
int open_safe(const char *path, const char *name);
int checkdir(const char *path, const char *dir, int create, int allowabs);

#endif /* OBEXFTP_IO_H */
