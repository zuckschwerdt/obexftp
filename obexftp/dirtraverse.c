/*
 *  obexftp/dirtraverse.c: ObexFTP directory recursion helper
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
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#define lstat(f,b) stat(f,b)
#define PATH_MAX MAX_PATH
#endif /* _WIN32 */
#include <dirent.h>

#include "dirtraverse.h"
#include <common.h>

/*  */
/* Read all files in a directory. Continue recusively down in directories. */
/*  */
int visit_dir(const char *path, const visit_cb cb, void *userdata)
{
	struct stat statbuf;
	DIR *dir;
	struct dirent *dirent;
	char *t;
	int len;
	int ret = 1;

	if((t = malloc(PATH_MAX + 1)) == NULL)
		return -1;
	t[PATH_MAX] = '\0'; /* save guard for strn... */
	
	dir = opendir(path);
	if(dir == NULL) {
		return -1;
	}
	while((dirent = readdir(dir)) != NULL) {
		if((strcmp(".", dirent->d_name) == 0) ||
		   (strcmp("..", dirent->d_name) == 0)) {
			continue;
		}

		strncpy(t, path, PATH_MAX);
		strncat(t, "/", PATH_MAX);
		strncat(t, dirent->d_name, PATH_MAX);
		if(lstat(t, &statbuf) < 0) {
			return -1;
		}
		else if(S_ISREG(statbuf.st_mode)) {
			ret = cb(VISIT_FILE, t, "", userdata);
			if( ret  < 0)
				goto out;
		}			
		else if(S_ISDIR(statbuf.st_mode)) {
			ret = cb(VISIT_GOING_DEEPER, dirent->d_name, path, userdata);
			if( ret < 0)
				goto out;
			len = strlen(t);
			strncat(t, dirent->d_name, PATH_MAX);
			strncat(t, "/", PATH_MAX);
			ret = visit_dir(t, cb, userdata);
			if(ret < 0)
				goto out;
			ret = cb(VISIT_GOING_UP, "", "", userdata);
			if(ret < 0)
				goto out;
			t[len] = '\0';
		}
	}

out:
	free(t);
	return ret;
}

/*  */
int visit_all_files(const char *path, visit_cb cb, void *userdata)
{
	struct stat statbuf;

	if(stat(path, &statbuf) < 0) {
		DEBUG(1, "Error stating %s\n", path);
		return -1;
	}

	if(S_ISREG(statbuf.st_mode)) {
		/* A single file. Just visit it, then we are done. */
		return cb(VISIT_FILE, path, "", userdata);
	}

	if(S_ISDIR(statbuf.st_mode)) {
		int ret;
		/* A directory! Enter it */
		
		/* Don't notify app if going "down" to "." */
		if(strcmp(path, ".") != 0) {
			ret = cb(VISIT_GOING_DEEPER, path, "", userdata);
			if(ret < 0)
				return ret;
		}

		ret = visit_dir(path, cb, userdata);
		if(ret < 0)
			return ret;

		ret = cb(VISIT_GOING_UP, "", "", userdata);
		if(ret < 0)
			return ret;
		return 0;
	}

	/* Not file, not dir, don't know what to do */
	return -1;
}

#ifdef UNIT_TEST
int visit(int action, const char *filename, char *path, void *userdata)
{
	switch(action) {
	case VISIT_FILE:
		DEBUG(2, "Visiting %s\n", filename);
		break;

	case VISIT_GOING_DEEPER:
		DEBUG(2, "Going deeper %s\n", filename);
		break;

	case VISIT_GOING_UP:
		DEBUG(2, "Going up\n");
		break;
	default:
		DEBUG(2, "going %d\n", action);
	}
	return 1;
}


/*  */
int main(int argc, char *argv[])
{
/* 	visit_all_files("Makefile", visit); */
/* 	visit_all_files("/usr/local/apache/", visit); */
	visit_all_files("testdir", visit);
	return 0;
}
#endif
