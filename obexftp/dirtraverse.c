
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>	/* FIXME: libraries shouldn't do this */
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#define lstat(f,b) stat(f,b)
#define PATH_MAX MAX_PATH
#endif /* _WIN32 */
#include <dirent.h>

#include "dirtraverse.h"
#include <common.h>

//
// Read all files in a directory. Continue recusively down in directories.
//
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
	
	dir = opendir(path);
	if(dir == NULL) {
		return -1;
	}
	dirent = readdir(dir);
	while(dirent != NULL) {
		if(strcmp(".", dirent->d_name) == 0) {
		}
		else if(strcmp("..", dirent->d_name) == 0) {
		}
		else {
			snprintf(t, PATH_MAX, "%s/%s", path, dirent->d_name);
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
			else {
				// This was probably a symlink. Just skip
			}
		}
		dirent = readdir(dir);
	}

out:	free(t);
	return ret;
}

//
//
//
int visit_all_files(const char *path, visit_cb cb, void *userdata)
{
	struct stat statbuf;
	int ret;

	if(stat(path, &statbuf) < 0) {
		DEBUG(1, "Error stating %s", path);
		ret = -1;
		goto out;
	}

	if(S_ISREG(statbuf.st_mode)) {
		/* A single file. Just visit it, then we are done. */
		ret = cb(VISIT_FILE, path, "", userdata);
	}
	else if(S_ISDIR(statbuf.st_mode)) {
		/* A directory! Enter it */
		
		/* Don't notify app if going "down" to "." */
		if(strcmp(path, ".") == 0)
			ret = 1;
		else
			ret = cb(VISIT_GOING_DEEPER, path, "", userdata);

		if(ret < 0)
			goto out;
		ret = visit_dir(path, cb, userdata);
		if(ret < 0)
			goto out;
		ret = cb(VISIT_GOING_UP, "", "", userdata);
		if(ret < 0)
			goto out;
	}
	else {
		/* Not file, not dir, don't know what to do */
		ret = -1;
	}

out:
	return ret;
}

#if 0
int visit(int action, const char *filename, char *path, void *userdata)
{
	switch(action) {
	case VISIT_FILE:
		DEBUG(2, "Visiting %s", filename);
		break;

	case VISIT_GOING_DEEPER:
		DEBUG(2, "Going deeper %s", filename);
		break;

	case VISIT_GOING_UP:
		DEBUG(2, "Going up");
		break;
	default:
		DEBUG(2, "going %d", action);
	}
	return 1;
}


//
//
//
int main(int argc, char *argv[])
{
//	visit_all_files("Makefile", visit);
//	visit_all_files("/usr/local/apache/", visit);
	visit_all_files("testdir", visit);
	return 0;
}
#endif
