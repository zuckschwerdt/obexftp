/*
 *  obexftp/object.c: ObexFTP library
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

#include <string.h>
#include <stdlib.h>

#include "object.h"

obex_object_t *obexftp_build_info (obex_t obex, uint32_t conn, uint8_t opcode)
{
	obex_object_t *object = NULL;
	uint8_t cmdstr[] = {APPARAM_INFO_CODE, 0x01, 0x00};

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff)
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, (obex_headerdata_t) conn, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
 
        cmdstr[2] = opcode;
	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_APPARAM, (obex_headerdata_t) (const uint8_t *) cmdstr, sizeof(cmdstr), OBEX_FL_FIT_ONE_PACKET);
 
	return object;
}


/* name and type musn't both be null */
obex_object_t *obexftp_build_get (obex_t obex, uint32_t conn, const char *name, const char *type)
{
	obex_object_t *object = NULL;
        uint8_t *ucname;
        int ucname_len;

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff)
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, (obex_headerdata_t) conn, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
 
        if(type != NULL) {
		// type header is a null terminated ascii string
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_TYPE, (obex_headerdata_t) (const uint8_t *) type, strlen(type)+1, OBEX_FL_FIT_ONE_PACKET);
	}	
 
	if (name != NULL) {
		ucname_len = strlen(name)*2 + 2;
		ucname = malloc(ucname_len);
		if(ucname == NULL) {
	                (void) OBEX_ObjectDelete(obex, object);
		        return NULL;
		}

		ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, (obex_headerdata_t) (const uint8_t *) ucname, ucname_len, OBEX_FL_FIT_ONE_PACKET);
		free(ucname);
	}
	
	return object;
}


/* neither filename may be null */
obex_object_t *obexftp_build_rename (obex_t obex, uint32_t conn, const char *from, const char *to)
{
	obex_object_t *object = NULL;
        uint8_t *appstr;
        uint8_t *appstr_p;
        int appstr_len;
        int ucname_len;
	char opname[] = {'m','o','v','e'};

        if((from == NULL) || (to == NULL))
                return NULL;

        object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff)
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, (obex_headerdata_t) conn, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);

        appstr_len = 1 + 1 + sizeof(opname) +
		strlen(from)*2 + 2 +
		strlen(to)*2 + 2 + 2;
        appstr = malloc(appstr_len);
        if(appstr == NULL) {
               	(void) OBEX_ObjectDelete(obex, object);
	        return NULL;
	}

	appstr_p = appstr;
	*appstr_p++ = 0x34;
	*appstr_p++ = sizeof(opname);
	memcpy(appstr_p, opname, sizeof(opname));
	appstr_p += sizeof(opname);

	*appstr_p++ = 0x35;
        ucname_len = OBEX_CharToUnicode(appstr_p + 1, from, strlen(from)*2 + 2);
	*appstr_p = ucname_len - 2; /* no trailing 0 */
	appstr_p += ucname_len - 1;

	*appstr_p++ = 0x36;
        ucname_len = OBEX_CharToUnicode(appstr_p + 1, to, strlen(to)*2 + 2);
	*appstr_p = ucname_len - 2; /* no trailing 0 */
	
        (void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_APPARAM, (obex_headerdata_t) (const uint8_t *) appstr, appstr_len - 2, 0);
        free(appstr);
	
	return object;
}


/* name may not be null */
obex_object_t *obexftp_build_del (obex_t obex, uint32_t conn, const char *name)
{
	obex_object_t *object;
        uint8_t *ucname;
        int ucname_len;

        if(name == NULL)
                return NULL;

        object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff)
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, (obex_headerdata_t) conn, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);

        ucname_len = strlen(name)*2 + 2;
        ucname = malloc(ucname_len);
        if(ucname == NULL) {
               	(void) OBEX_ObjectDelete(obex, object);
	        return NULL;
	}

        ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

        (void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, (obex_headerdata_t) (const uint8_t *) ucname, ucname_len, OBEX_FL_FIT_ONE_PACKET);
        free(ucname);
	
	return object;
}


/* if name is null ascend one directory */
/* if name is empty change to top/default directory */
obex_object_t *obexftp_build_setpath (obex_t obex, uint32_t conn, const char *name, int create)
{
	obex_object_t *object;
	// "Backup Level" and "Don't Create" flag in first byte
	// second byte is reserved and needs to be 0
	uint8_t setpath_nohdr_data[2] = {0, 0};
	char *ucname;
	int ucname_len;

	object = OBEX_ObjectNew(obex, OBEX_CMD_SETPATH);
	if(object == NULL)
		return NULL;

        if(conn != 0xffffffff)
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, (obex_headerdata_t) conn, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);

	if (create == 0) {
		// set the 'Don't Create' bit
		setpath_nohdr_data[0] |= 2;
	}
	if (name) {
		ucname_len = strlen(name)*2 + 2;
		ucname = malloc(ucname_len);
		if (ucname == NULL) {
			(void) OBEX_ObjectDelete(obex, object);
			return NULL;
		}
		ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

		/* apparently the empty name header is meant to be really empty... */
		if (ucname_len == 2)
			ucname_len = 0;

		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, (obex_headerdata_t) (const uint8_t *) ucname, ucname_len, 0);
		free(ucname);
	}
	else {
		setpath_nohdr_data[0] = 1; /* or |= perhaps? */
	}

	(void) OBEX_ObjectSetNonHdrData(object, setpath_nohdr_data, 2);

	return object;
}


/* use build_object_from_file() instead */
obex_object_t *obexftp_build_put (obex_t obex, uint32_t conn, const char *name, const int size)
{
	obex_object_t *object = NULL;
	uint8_t *ucname;
	int ucname_len;
		
	object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
	if(object == NULL)
		return NULL;

        if(conn != 0xffffffff)
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, (obex_headerdata_t) conn, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);

	ucname_len = strlen(name)*2 + 2;
	ucname = malloc(ucname_len);
	if(ucname == NULL) {
       		(void) OBEX_ObjectDelete(obex, object);
		return NULL;
	}

	ucname_len = OBEX_CharToUnicode(ucname, name, ucname_len);

	(void ) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, (obex_headerdata_t) (const uint8_t *) ucname, ucname_len, 0);
	free(ucname);

	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_LENGTH, (obex_headerdata_t) (uint32_t)size, sizeof(uint32_t), 0);

	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_BODY,
				(obex_headerdata_t) (const uint8_t *) NULL,
				0, OBEX_FL_STREAM_START);

	return object;
}
