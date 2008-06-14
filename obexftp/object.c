/**
	\file obexftp/object.c
	Collection of functions to build common OBEX request objects.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unicode.h"
#include "object.h"


/**
	Build an INFO request object (Siemens only).

	\param obex reference to an OpenOBEX instance.
	\param conn optional connection id number
	\param opcode select if you want to read
	 mem installed (0x01) or free mem (0x02)
	\return a new obex object if successful, NULL otherwise
 */
obex_object_t *obexftp_build_info (obex_t obex, uint32_t conn, uint8_t opcode)
{
	obex_object_t *object;
	obex_headerdata_t hv;
	uint8_t cmdstr[] = {APPARAM_INFO_CODE, 0x01, 0x00};

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}
 
        cmdstr[2] = opcode;
	hv.bs = (const uint8_t *) cmdstr;
	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_APPARAM, hv, sizeof(cmdstr), OBEX_FL_FIT_ONE_PACKET);
 
	return object;
}


/**
	Build a GET request object.

	\param obex reference to an OpenOBEX instance.
	\param conn optional connection id number
	\param name name of the requested file
	\param type type of the requested file
	\return a new obex object if successful, NULL otherwise

	\note \a name and \a type musn't both be NULL
 */
obex_object_t *obexftp_build_get (obex_t obex, uint32_t conn, const char *name, const char *type)
{
	obex_object_t *object;
	obex_headerdata_t hv;
        uint8_t *ucname;
        int ucname_len;

        object = OBEX_ObjectNew(obex, OBEX_CMD_GET);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}
 
        if(type != NULL) {
		// type header is a null terminated ascii string
		hv.bs = (const uint8_t *) type;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_TYPE, hv, strlen(type)+1, OBEX_FL_FIT_ONE_PACKET);
	}	
 
	if (name != NULL) {
		ucname_len = strlen(name)*2 + 2;
		ucname = malloc(ucname_len);
		if(ucname == NULL) {
	                (void) OBEX_ObjectDelete(obex, object);
		        return NULL;
		}

		ucname_len = CharToUnicode(ucname, (uint8_t*)name, ucname_len);

		hv.bs = (const uint8_t *) ucname;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hv, ucname_len, OBEX_FL_FIT_ONE_PACKET);
		free(ucname);
	}
	
	return object;
}


/**
	Build a RENAME request object (Siemens only).

	\param obex reference to an OpenOBEX instance.
	\param conn optional connection id number
	\param from original name of the requested file
	\param to new name of the requested file
	\return a new obex object if successful, NULL otherwise

	\note neither filename may be NULL
 */
obex_object_t *obexftp_build_rename (obex_t obex, uint32_t conn, const char *from, const char *to)
{
	obex_object_t *object;
	obex_headerdata_t hv;
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

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}

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
        ucname_len = CharToUnicode(appstr_p + 1, (uint8_t*)from, strlen(from)*2 + 2);
	*appstr_p = ucname_len - 2; /* no trailing 0 */
	appstr_p += ucname_len - 1;

	*appstr_p++ = 0x36;
        ucname_len = CharToUnicode(appstr_p + 1, (uint8_t*)to, strlen(to)*2 + 2);
	*appstr_p = ucname_len - 2; /* no trailing 0 */
	
        hv.bs = (const uint8_t *) appstr;
        (void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_APPARAM, hv, appstr_len - 2, 0);
        free(appstr);
	
	return object;
}


/**
	Build a DELETE request object.

	\param obex reference to an OpenOBEX instance.
	\param conn optional connection id number
	\param name name of the file to be deleted
	\return a new obex object if successful, NULL otherwise

	\note \a name may not be NULL
 */
obex_object_t *obexftp_build_del (obex_t obex, uint32_t conn, const char *name)
{
	obex_object_t *object;
	obex_headerdata_t hv;
        uint8_t *ucname;
        int ucname_len;

        if(name == NULL)
                return NULL;

        object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
        if(object == NULL)
                return NULL;

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}

        ucname_len = strlen(name)*2 + 2;
        ucname = malloc(ucname_len);
        if(ucname == NULL) {
               	(void) OBEX_ObjectDelete(obex, object);
	        return NULL;
	}

        ucname_len = CharToUnicode(ucname, (uint8_t*)name, ucname_len);

        hv.bs = (const uint8_t *) ucname;
        (void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hv, ucname_len, OBEX_FL_FIT_ONE_PACKET);
        free(ucname);
	
	return object;
}


/**
	Build a SETPATH request object.

	\param obex reference to an OpenOBEX instance.
	\param conn optional connection id number
	\param name name of the file to be deleted
	\param create create the folder if neccessary
	\return a new obex object if successful, NULL otherwise

	\note 
	 if \a name is NULL ascend one directory 
	 if \a name is empty change to top/default directory
 */
obex_object_t *obexftp_build_setpath (obex_t obex, uint32_t conn, const char *name, int create)
{
	obex_object_t *object;
	obex_headerdata_t hv;
	// "Backup Level" and "Don't Create" flag in first byte
	// second byte is reserved and needs to be 0
	uint8_t setpath_nohdr_data[2] = {0, 0};
        uint8_t *ucname;
	int ucname_len;

	object = OBEX_ObjectNew(obex, OBEX_CMD_SETPATH);
	if(object == NULL)
		return NULL;

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}

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
		ucname_len = CharToUnicode(ucname, (uint8_t*)name, ucname_len);

		/* apparently the empty name header is meant to be really empty... */
		if (ucname_len == 2)
			ucname_len = 0;

        	hv.bs = (const uint8_t *) ucname;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hv, ucname_len, 0);
		free(ucname);
	}
	else {
		setpath_nohdr_data[0] = 1; /* or |= perhaps? */
	}

	(void) OBEX_ObjectSetNonHdrData(object, setpath_nohdr_data, 2);

	return object;
}


/**
	Build a PUT request object.

	\param obex reference to an OpenOBEX instance.
	\param conn optional connection id number
	\param name name of the target file
	\param size size hint for the target file
	\return a new obex object if successful, NULL otherwise

	\note use build_object_from_file() instead
 */
obex_object_t *obexftp_build_put (obex_t obex, uint32_t conn, const char *name, const int size)
{
	obex_object_t *object;
	obex_headerdata_t hv;
	uint8_t *ucname;
	int ucname_len;
		
	object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
	if(object == NULL)
		return NULL;

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}

	ucname_len = strlen(name)*2 + 2;
	ucname = malloc(ucname_len);
	if(ucname == NULL) {
       		(void) OBEX_ObjectDelete(obex, object);
		return NULL;
	}

	ucname_len = CharToUnicode(ucname, (uint8_t*)name, ucname_len);

       	hv.bs = (const uint8_t *) ucname;
	(void ) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hv, ucname_len, 0);
	free(ucname);

	hv.bq4 = (uint32_t) size;
	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);

	hv.bs = (const uint8_t *) NULL;
	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_BODY, hv, 0, OBEX_FL_STREAM_START);

	return object;
}
