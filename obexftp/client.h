#ifndef OBEXFTP_CLIENT_H
#define OBEXFTP_CLIENT_H

#include <stdint.h>
#include <openobex/obex.h>
#include "obexftp.h"

typedef struct obexftp_client
{
	obex_t *obexhandle;
	int finished;
	int success;
	int obex_rsp;
	obexftp_info_cb_t infocb;
	void *infocb_data;
	int fd;
	int out_fd;
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

int obexftp_setpath(obexftp_client_t *cli,
		    /*@null@*/ const char *name,
		    int up);

int obexftp_put(obexftp_client_t *cli, const char *name);

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

#endif /* OBEXFTP_CLIENT_H */
