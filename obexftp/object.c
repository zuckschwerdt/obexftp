
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <openobex/obex.h>

#include <common.h>

#include "object.h"

obex_object_t *obexftp_build_info (obex_t obex, uint8_t opcode)
{
	obex_object_t *object = NULL;
        obex_headerdata_t hdd;
	uint8_t cmdstr[] = {APPARAM_INFO_CODE, 0x01, 0x00};

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;
 
        cmdstr[2] = opcode;
        hdd.bs = cmdstr;
	OBEX_ObjectAddHeader(obex, object, OBEX_HDR_APPARAM, hdd, sizeof(cmdstr), OBEX_FL_FIT_ONE_PACKET);
 
	return object;
}

//	cli->out_fd = STDOUT_FILENO;
obex_object_t *obexftp_build_list (obex_t obex, const char *name)
{
	obex_object_t *object = NULL;
        obex_headerdata_t hdd;
        uint8_t *ucname;
        int ucname_len;

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;
 
        hdd.bs = XOBEX_LISTING;
	OBEX_ObjectAddHeader(obex, object, OBEX_HDR_TYPE, hdd, sizeof(XOBEX_LISTING), OBEX_FL_FIT_ONE_PACKET);
 
	if (name != NULL) {
		ucname_len = strlen(name)*2 + 2;
		ucname = malloc(ucname_len);
		if(ucname == NULL)
			goto err;

		ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

		hdd.bs = ucname;
		OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hdd, ucname_len, OBEX_FL_FIT_ONE_PACKET);
		free(ucname);
	}
	
	return object;

err:
        if(object != NULL)
                OBEX_ObjectDelete(obex, object);
        return NULL;
}


//	cli->out_fd = open_safe("", localname);
obex_object_t *obexftp_build_get (obex_t obex, const char *name)
{
	obex_object_t *object = NULL;
        obex_headerdata_t hdd;
        uint8_t *ucname;
        int ucname_len;

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;

        ucname_len = strlen(name)*2 + 2;
        ucname = malloc(ucname_len);
        if(ucname == NULL)
                goto err;

        ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

        hdd.bs = ucname;
        OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hdd, ucname_len, 0);
        free(ucname);
	
	return object;

err:
        if(object != NULL)
                OBEX_ObjectDelete(obex, object);
        return NULL;
}


obex_object_t *obexftp_build_rename (obex_t obex, const char *from, const char *to)
{
	obex_object_t *object = NULL;
        obex_headerdata_t hdd;
        uint8_t *appstr;
        uint8_t *appstr_p;
        int appstr_len;
        int ucname_len;
	char opname[] = {'m','o','v','e'};

        object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
        if(object == NULL)
                return NULL;

        appstr_len = 1 + 1 + sizeof(opname) +
		strlen(from)*2 + 2 +
		strlen(to)*2 + 2 + 2;
        appstr = malloc(appstr_len);
        if(appstr == NULL)
                goto err;

	appstr_p = appstr;
	*appstr_p++ = 0x34;
	*appstr_p++ = sizeof(opname);
	memcpy(appstr_p, opname, sizeof(opname));
	appstr_p += sizeof(opname);

	*appstr_p++ = 0x35;
        ucname_len = OBEX_CharToUnicode(appstr_p + 1, from, strlen(from)*2 + 2);
	*appstr_p = ucname_len - 2; // no trailing 0
	appstr_p += ucname_len - 1;

	*appstr_p++ = 0x36;
        ucname_len = OBEX_CharToUnicode(appstr_p + 1, to, strlen(to)*2 + 2);
	*appstr_p = ucname_len - 2; // no trailing 0
	
        hdd.bs = appstr;
        OBEX_ObjectAddHeader(obex, object, OBEX_HDR_APPARAM, hdd, appstr_len - 2, 0);
        free(appstr);
	
	return object;

err:
        if(object != NULL)
                OBEX_ObjectDelete(obex, object);
        return NULL;
}


obex_object_t *obexftp_build_del (obex_t obex, const char *name)
{
	obex_object_t *object;
        obex_headerdata_t hdd;
        uint8_t *ucname;
        int ucname_len;

        object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
        if(object == NULL)
                return NULL;

        ucname_len = strlen(name)*2 + 2;
        ucname = malloc(ucname_len);
        if(ucname == NULL)
                goto err;

        ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

        hdd.bs = ucname;
        OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hdd, ucname_len, OBEX_FL_FIT_ONE_PACKET);
        free(ucname);
	
	return object;

err:
        if(object != NULL)
                OBEX_ObjectDelete(obex, object);
        return NULL;
}


obex_object_t *obexftp_build_setpath (obex_t obex, const char *name, int up)
{
	obex_object_t *object;
	obex_headerdata_t hdd;
	uint8_t setpath_nohdr_data[2] = {0,};
	char *ucname;
	int ucname_len;

	object = OBEX_ObjectNew(obex, OBEX_CMD_SETPATH);

	if(up) {
		setpath_nohdr_data[0] = 1;
	}
	else {
		ucname_len = strlen(name)*2 + 2;
		ucname = malloc(ucname_len);
		if(ucname == NULL) {
			OBEX_ObjectDelete(obex, object);
			return NULL;
		}
		ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

		hdd.bs = ucname;
		OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hdd, ucname_len, 0);
		free(ucname);
	}

	OBEX_ObjectSetNonHdrData(object, setpath_nohdr_data, 2);

	return object;
}


obex_object_t *obexftp_build_put (obex_t obex, const char *name)
{
	return NULL;
}
