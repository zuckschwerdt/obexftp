/*
 *  obexftp/client.i: ObexFTP client library SWIG interface
 *
 *  Copyright (c) 2005 Christian W. Zuckschwerdt <zany@triq.net>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */

%module OBEXFTP
%{
#include "obexftp.h"
#include "client.h"
%}
     
%name(client) typedef struct {
} obexftp_client_t;

%extend obexftp_client_t {

%constant int IRDA = OBEX_TRANS_IRDA;
%constant int INET = OBEX_TRANS_INET;
%constant int CABLE = OBEX_TRANS_CUSTOM;
%constant int BLUETOOTH = OBEX_TRANS_BLUETOOTH;
%constant int USB = OBEX_TRANS_USB;

obexftp_client_t(int transport) {
	return obexftp_open(transport, NULL, NULL, NULL);
}
~obexftp_client_t() {
	obexftp_close(self);
}

int connect(char *device, int port) {
	obexftp_connect_uuid(self, device, port, UUID_FBS);
}
int disconnect() {
	obexftp_disconnect(self);
}

int chpath(char *name) {
	return obexftp_setpath(self, name, 0);
}
int mkpath(char *name) {
	return obexftp_setpath(self, name, 1);
}
int cdup() {
	return obexftp_setpath(self, NULL, 0);
}
int cdtop() {
	return obexftp_setpath(self, "", 0);
}

char *get(char *path) {
	(void) obexftp_get_type(self, NULL, NULL, path);
	return self->buf_data;
}
char *list(char *path) {
	(void) obexftp_get_type(self, XOBEX_LISTING, NULL, path);
	return self->buf_data;
}
char *get_capability(char *path) {
	(void) obexftp_get_type(self, XOBEX_CAPABILITY, NULL, path);
	return self->buf_data;
}

int put(char *filename) {
	return obexftp_put(self, filename);
}
int put_to(char *filename, char *remotename) {
	return obexftp_put_file(self, filename, remotename);
}
int put_data(char *data, char *remotename) {
	// Danger Wil Robinson
	return obexftp_put_data(self, data, strlen(data), remotename);
}

int delete(char *name) {
	return obexftp_del(self, name);
}

}

