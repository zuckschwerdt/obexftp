/*
 *  obexftp/obexftp_io.c: ObexFTP IO abstraction
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

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>	/* FIXME: libraries shouldn't do this */
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
#define PATH_MAX MAX_PATH
#endif
#define DEFFILEMOD (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) /* 0644 */
#define DEFXFILEMOD (DEFFILEMOD | S_IXGRP | S_IXUSR | S_IXOTH) /* 0755 */

/* Get some file-info. (size and lastmod) */
static int get_fileinfo(const char *name, char *lastmod)
{
	struct stat stats;
	struct tm *tm;
	
	stat(name, &stats);
	tm = gmtime(&stats.st_mtime);
	snprintf(lastmod, 21, "%04d-%02d-%02dT%02d:%02d:%02dZ",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	return (int) stats.st_size;
}


/* Create an object from a file. Attach some info-headers to it */
obex_object_t *build_object_from_file(obex_t *handle, const char *localname, const char *remotename)
{
	obex_object_t *object = NULL;
	obex_headerdata_t hdd;
	uint8_t *ucname;
	int ucname_len, size;
	char lastmod[21*2] = {"1970-01-01T00:00:00Z"};
		
	/* Get filesize and modification-time */
	size = get_fileinfo(localname, lastmod);

	object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	if(object == NULL)
		return NULL;

	ucname_len = strlen(remotename)*2 + 2;
	ucname = malloc(ucname_len);
	if(ucname == NULL) {
		if(object != NULL)
			OBEX_ObjectDelete(handle, object);
		return NULL;
	}

	ucname_len = OBEX_CharToUnicode(ucname, remotename, ucname_len);

	hdd.bs = ucname;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hdd, ucname_len, 0);
	free(ucname);

	hdd.bq4 = size;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hdd, sizeof(uint32_t), 0);

#if 0
	/* Win2k excpects this header to be in unicode. I suspect this in
	   incorrect so this will have to wait until that's investigated */
	hdd.bs = lastmod;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_TIME, hdd, strlen(lastmod)+1, 0);
#endif
		
	hdd.bs = NULL;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
				hdd, 0, OBEX_FL_STREAM_START);

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
/* dest must have at least PATH_MAX + 1 chars. */
char *pathcat(char *dest, const char *path, const char *name)
{
	if(name == NULL)
		return dest;
		
	while(*name == '/')
		name++;
		
	if((path == NULL) || (*path == '\0'))
		strncpy(dest, name, PATH_MAX);
	else {
		strncpy(dest, path, PATH_MAX);
		if (dest[strlen(dest)-1] != '/') {
			dest[strlen(dest) + 1] = '\0';
			dest[strlen(dest)] = '/';
		}
		strncat(dest, name, PATH_MAX-strlen(dest));
	}

	return dest;
}
	
/* Open a file, but do some sanity-checking first. */
int open_safe(const char *path, const char *name)
{
	char diskname[PATH_MAX + 1] = {0,};
	int fd;

	DEBUG(3, "%s() \n", __func__);
	
	/* Check for dangerous filenames */
	if(nameok(name) == FALSE)
		return -1;

	/* TODO! Rename file if already exist. */

	pathcat(diskname, path, name);

	DEBUG(3, "%s() Creating file %s\n", __func__, diskname);

	fd = open(diskname, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMOD);
	return fd;
}

/* Go to a directory. Create if not exists and create is true. */
int checkdir(const char *path, const char *dir, cd_flags flags)
{
	char newpath[PATH_MAX + 1] = {0,};
	struct stat statbuf;
	int ret = -1;

	if(!(flags & CD_ALLOWABS))	{
		if(nameok(dir) == FALSE)
			return -1;
	}

	pathcat(newpath, path, dir);

	DEBUG(3, "%s() path = %s dir = %s, flags = %d\n", __func__, path, dir, flags);
	if(stat(newpath, &statbuf) == 0) {
		/* If this directory aleady exist we are done */
		if(S_ISDIR(statbuf.st_mode)) {
			DEBUG(3, "%s() Using existing dir\n", __func__);
			return 1;
		}
		else  {
			/* A non-directory with this name already exist. */
			DEBUG(3, "%s() A non-dir called %s already exist\n", __func__, newpath);
			return -1;
		}
	}
	if(flags & CD_CREATE) {
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

	return ret;
}
	
