/*
 *  obexftp/client.c: ObexFTP client library
 *
 *  Copyright (c) 2002 Christian W. Zuckschwerdt <zany@triq.net>
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

#ifndef OBEXFTP_CLIENT_H
#define OBEXFTP_CLIENT_H

#ifdef SWIG
%module example
%{
#include "obexftp.h"
#include "client.h"
#include <stdint.h>
%}
#endif
     
#include <stdint.h>
#include <openobex/obex.h>
#include "obexftp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct obexftp_client
{
	obex_t *obexhandle;
	int finished;
	int success;
	int obex_rsp;
	obexftp_info_cb_t infocb;
	void *infocb_data;
	int fd; /* used in put body */
	char *target_fn; /* used in get body */
	uint8_t *buf;
} obexftp_client_t;

/* session */

int obexftp_sync(obexftp_client_t *cli);

obexftp_client_t *obexftp_cli_open(/*@null@*/ obexftp_info_cb_t infocb,
				   /*@null@*/ /*const*/ obex_ctrans_t *ctrans,
				   /*@null@*/ void *infocb_data);

void obexftp_cli_close(obexftp_client_t *cli);

int obexftp_cli_connect(obexftp_client_t *cli);

int obexftp_cli_disconnect(obexftp_client_t *cli);

/* transfer */

int obexftp_setpath(obexftp_client_t *cli, /*@null@*/ const char *name);

int obexftp_put(obexftp_client_t *cli, const char *name);

int obexftp_put_file(obexftp_client_t *cli, const char *localname,
		     const char *remotename);

int obexftp_del(obexftp_client_t *cli, const char *name);

int obexftp_info(obexftp_client_t *cli, uint8_t opcode);

int obexftp_list(obexftp_client_t *cli,
		 /*@null@*/ const char *localname,
		 /*@null@*/ const char *remotename);

int obexftp_get(obexftp_client_t *cli,
		/*@null@*/  const char *localname,
		const char *remotename);

int obexftp_rename(obexftp_client_t *cli,
		   const char *sourcename,
		   const char *targetname);

#ifdef __cplusplus
}
#endif

#endif /* OBEXFTP_CLIENT_H */
