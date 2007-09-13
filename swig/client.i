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

/* perl croaks if this is lowercase. override for every other binding. */
%module OBEXFTP
%{
#include <obexftp/obexftp.h>
#include <obexftp/client.h>
%}

%include "charmap.i"

%constant int IRDA = OBEX_TRANS_IRDA;
%constant int INET = OBEX_TRANS_INET;
%constant int CABLE = OBEX_TRANS_CUSTOM;
%constant int BLUETOOTH = OBEX_TRANS_BLUETOOTH;
%constant int USB = OBEX_TRANS_USB;

%constant int SYNC = OBEX_SYNC_SERVICE;
%constant int PUSH = OBEX_PUSH_SERVICE;
%constant int FTP = OBEX_FTP_SERVICE;

%rename(discover) obexftp_discover;
char **obexftp_discover(int transport);

%rename(browsebt) obexftp_browse_bt;
int obexftp_browse_bt(char *addr, int service);


#if defined SWIGPERL
#elif defined SWIGPYTHON
%typemap(in) (obexftp_info_cb_t infocb, void *user_data) {
        if (!PyCallable_Check($input)) {
                /* should raise an exception here */
                $1 = NULL;
                $2 = NULL;
        } else {
                Py_XINCREF($input);
                $1 = proxy_info_cb;
                $2 = $input;
        }
};
%{
static void proxy_info_cb (int evt, const char *buf, int len, void *data) {
        PyObject *proc = (PyObject *)data;
        /* PyObject *msg = PyString_FromStringAndSize(buf, len); */
        PyObject_CallFunction(proc, "is", evt, buf);
}
%} 
#elif defined SWIGRUBY
%typemap(in) (obexftp_info_cb_t infocb, void *user_data) {
	$1 = proxy_info_cb;
	$2 = $input;
};

%{
static void proxy_info_cb (int event, const char *buf, int len, void *data) {
  VALUE proc = (VALUE)data;
  VALUE msg = buf ? rb_str_new(buf, len) : Qnil;
  rb_funcall(proc, rb_intern("call"), 2, INT2NUM(event), msg);
}
%}
#elif defined SWIGTCL
#else
#warning "no callbacks for this language"
#endif


/* Which binding wants this capitalized too? */
%rename(client) obexftp_client_t;
#ifdef SWIGRUBY
%rename(Client) obexftp_client_t;
#endif
typedef struct {
} obexftp_client_t;

%extend obexftp_client_t {

obexftp_client_t(int transport) {
	return obexftp_open(transport, NULL, NULL, NULL);
}
~obexftp_client_t() {
	obexftp_close(self);
}

void callback(obexftp_info_cb_t infocb, void *user_data) {
	self->infocb = infocb;
	self->infocb_data = user_data;
}

char **discover() {
	return obexftp_discover(self->transport);
}

int connect(char *device, int port, char *src=NULL) {
	return obexftp_connect_src(self, src, device, port, UUID_FBS, sizeof(UUID_FBS));
}
int connectpush(char *device, int port, char *src=NULL) {
	self->quirks &= ~OBEXFTP_SPLIT_SETPATH;
	return obexftp_connect_src(self, src, device, port, NULL, 0);
}
int connectsync(char *device, int port, char *src=NULL) {
	self->quirks &= ~OBEXFTP_SPLIT_SETPATH;
	return obexftp_connect_src(self, src, device, port, UUID_IRMC, sizeof(UUID_IRMC));
}
int disconnect() {
	return obexftp_disconnect(self);
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
	return (char *)self->buf_data;
}
char *list(char *path=NULL) {
	(void) obexftp_get_type(self, XOBEX_LISTING, NULL, path);
	return (char *)self->buf_data;
}
char *get_capability(char *path=NULL) {
	(void) obexftp_get_type(self, XOBEX_CAPABILITY, NULL, path);
	return (char *)self->buf_data;
}

int get_file(char *path, char *localname) { 
        return obexftp_get_type(self, NULL, localname, path); 
} 
int put_file(char *filename, char *remotename=NULL) {
	return obexftp_put_file(self, filename, remotename);
}

int put_data(char *data, size_t size, char *remotename=NULL) {
	return obexftp_put_data(self, data, size, remotename);
}

int delete(char *name) {
	return obexftp_del(self, name);
}

}

