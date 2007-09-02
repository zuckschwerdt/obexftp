/**
	\file obexftp/obexftp_io.c
	ObexFTP IO abstraction implementation.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <openobex/obex.h>

#include "obexftp_io.h"
#include <common.h>

#ifdef _WIN32
#define S_IRGRP 0
#define S_IROTH 0
#define S_IXGRP 0
#define S_IXOTH 0
#endif
#define DEFFILEMOD (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) /* 0644 */
#define DEFXFILEMOD (DEFFILEMOD | S_IXGRP | S_IXUSR | S_IXOTH) /* 0755 */

/* Get some file-info. (size and lastmod) */
/* lastmod needs to have at least 21 bytes */
static int get_fileinfo(const char *name, char *lastmod)
{
	struct stat stats;
	struct tm *tm;
	
	if ((0 == stat(name, &stats)) &&
		((tm = gmtime(&stats.st_mtime)) != NULL)) {
		(void) snprintf(lastmod, 21, "%04d-%02d-%02dT%02d:%02d:%02dZ",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
		return (int) stats.st_size;
	}
	return -1;
}


/* Create an object from a file. Attach some info-headers to it */
obex_object_t *build_object_from_file(obex_t *obex, uint32_t conn, const char *localname, const char *remotename)
{
	obex_object_t *object;
	obex_headerdata_t hv;
	uint8_t *ucname;
	int ucname_len, size;
	char lastmod[] = "11997700--0011--0011TT0000::0000::0000ZZ.";
		
	/* Get filesize and modification-time */
	size = get_fileinfo(localname, lastmod);

	object = OBEX_ObjectNew(obex, OBEX_CMD_PUT);
	if(object == NULL)
		return NULL;

        if(conn != 0xffffffff) {
		hv.bq4 = conn;
		(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_CONNECTION, hv, sizeof(uint32_t), OBEX_FL_FIT_ONE_PACKET);
	}

	ucname_len = strlen(remotename)*2 + 2;
	ucname = malloc(ucname_len);
	if(ucname == NULL) {
		(void) OBEX_ObjectDelete(obex, object);
		return NULL;
	}

	ucname_len = OBEX_CharToUnicode(ucname, (uint8_t*)remotename, ucname_len);

	hv.bs = ucname;
	(void ) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_NAME, hv, ucname_len, 0);
	free(ucname);

	hv.bq4 = (const uint32_t) size;
	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);

#if 0
	/* Win2k excpects this header to be in unicode. I suspect this in
	   incorrect so this will have to wait until that's investigated */
	hv.bs = (const uint8_t *) lastmod;
	OBEX_ObjectAddHeader(obex, object, OBEX_HDR_TIME, hv, strlen(lastmod)+1, 0);
#endif
		
	hv.bs = (const uint8_t *) NULL;
	(void) OBEX_ObjectAddHeader(obex, object, OBEX_HDR_BODY,
				hv, 0, OBEX_FL_STREAM_START);

	DEBUG(3, "%s() Lastmod = %s\n", __func__, lastmod);
	return object;
}

/* Check for dangerous filenames. */
static int nameok(const char *name)
{
	DEBUG(3, "%s() \n", __func__);

        return_val_if_fail (name != NULL, FALSE);
	
	/* No abs paths */
	if(name[0] == '/')
		return FALSE;

	if(strlen(name) >= 3) {
		/* "../../vmlinuz" */
		if(name[0] == '.' && name[1] == '.' && name[2] == '/')
			return FALSE;
		/* "dir/../../../vmlinuz" */
		if(strstr(name, "/../") != NULL)
			return FALSE;
	}
	return TRUE;
}

/* Concatenate two pathnames. */
/* The first path may be NULL. */
/* The second path is always treated relative. */
static int pathncat(/*@unique@*/ char *dest, const char *path, const char *name, size_t n)
{
	size_t len;
	if(name == NULL)
		return -EINVAL;
		
	while(*name == '/')
		name++;
		
	if((path == NULL) || (*path == '\0')) {
		strncpy(dest, name, n);
		dest[n - 1] = '\0';
	} else {
		strncpy(dest, path, n);
		dest[n - 1] = '\0';
		len = strlen(dest);
		if (len >= n - 1)
			return -ENOMEM;
		if (dest[len - 1] != '/') {
			dest[len] = '/';
			dest[len + 1] = '\0';
		}
		strncat(dest, name, n - strlen(dest) - 1);
	}

	return 0;
}
	
/* Open a file, but do some sanity-checking first. */
int open_safe(const char *path, const char *name)
{
	char *diskname;
	size_t maxlen;
	int fd;

	DEBUG(3, "%s() \n", __func__);
	
	/* Check for dangerous filenames */
	if(nameok(name) == FALSE)
		return -1;

	/* TODO! Rename file if already exist. */

        maxlen = strlen(name) + 1;
	if (path)
        	maxlen += strlen(path);
	diskname = malloc(maxlen);
	if(!diskname)
		return -1;
	(void) pathncat(diskname, path, name, maxlen);

	DEBUG(3, "%s() Creating file %s\n", __func__, diskname);

	fd = open(diskname, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMOD);
	free(diskname);
	return fd;
}

/* Go to a directory. Create if not exists and create is true. */
int checkdir(const char *path, const char *dir, int create, int allowabs)
{
	char *newpath;
	size_t maxlen;
	struct stat statbuf;
	int ret = -1;

	if(!allowabs)	{
		if(nameok(dir) == FALSE)
			return -1;
	}

	if(!dir)
		return 1;
        maxlen = strlen(dir) + 1;
	if (path)
        	maxlen += strlen(path);
	newpath = malloc(maxlen);
	if(!newpath)
		return -1;
	(void) pathncat(newpath, path, dir, maxlen);

	DEBUG(3, "%s() path = %s dir = %s, create = %d, allowabs = %d\n", __func__, path, dir, create, allowabs);
	if(stat(newpath, &statbuf) == 0) {
		/* If this directory aleady exist we are done */
		if(S_ISDIR(statbuf.st_mode)) {
			DEBUG(3, "%s() Using existing dir\n", __func__);
			free(newpath);
			return 1;
		}
		else  {
			/* A non-directory with this name already exist. */
			DEBUG(3, "%s() A non-dir called %s already exist\n", __func__, newpath);
			free(newpath);
			return -1;
		}
	}
	if(create) {
		DEBUG(3, "%s() Will try to create %s\n", __func__, newpath);
#ifdef _WIN32
		ret = mkdir(newpath);
#else
		ret = mkdir(newpath, DEFXFILEMOD);
#endif
	}
	else {
		ret = -1;
	}

	free(newpath);
	return ret;
}
	
