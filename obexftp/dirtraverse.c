#include <glib.h>

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "dirtraverse.h"
#include <g_debug.h>

//
// Read all files in a directory. Continue recusively down in directories.
//
int visit_dir(GString *path, visit_cb cb, gpointer userdata)
{
	struct stat statbuf;
	DIR *dir;
	struct dirent *dirent;
	GString *t;
	int len;
	int ret = 1;

	t = g_string_new("");
	if(t == NULL)
		return -1;

	dir = opendir(path->str);
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
			g_string_sprintf(t, "%s/%s", path->str, dirent->d_name);
			if(lstat(t->str, &statbuf) < 0) {
				return -1;
			}
			else if(S_ISREG(statbuf.st_mode)) {
				ret = cb(VISIT_FILE, t->str, "", userdata);
				if( ret  < 0)
					goto out;
			}			
			else if(S_ISDIR(statbuf.st_mode)) {
				ret = cb(VISIT_GOING_DEEPER, dirent->d_name, path->str, userdata);
				if( ret < 0)
					goto out;
				len = path->len;
				g_string_append(path, dirent->d_name);
				g_string_append(path, "/");
				ret = visit_dir(t, cb, userdata);
				if(ret < 0)
					goto out;
				ret = cb(VISIT_GOING_UP, "", "", userdata);
				if(ret < 0)
					goto out;
				g_string_truncate(path, len);
			}
			else {
				// This was probably a symlink. Just skip
			}
		}
		dirent = readdir(dir);
	}

out:	g_string_free(t, TRUE);
	return ret;
}

//
//
//
gint visit_all_files(const gchar *name, visit_cb cb, gpointer userdata)
{
	struct stat statbuf;
	int ret;
	GString *path;
	path = g_string_new("");

	if(stat(name, &statbuf) < 0) {
		g_warning("Error stating %s", name);
		ret = -1;
		goto out;
	}

	if(S_ISREG(statbuf.st_mode)) {
		/* A single file. Just visit it, then we are done. */
		ret = cb(VISIT_FILE, name, "", userdata);
	}
	else if(S_ISDIR(statbuf.st_mode)) {
		/* A directory! Enter it */
		path = g_string_assign(path, name);
		
		/* Don't notify app if going "down" to "." */
		if(strcmp(name, ".") == 0)
			ret = 1;
		else
			ret = cb(VISIT_GOING_DEEPER, name, "", userdata);

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
	g_string_free(path, TRUE);
	return ret;
}

#if 0
gint visit(gint action, gchar *name, gchar *path, gpointer userdata)
{
	switch(action) {
	case VISIT_FILE:
		g_info("Visiting %s", filename);
		break;

	case VISIT_GOING_DEEPER:
		g_info("Going deeper %s", filename);
		break;

	case VISIT_GOING_UP:
		g_info("Going up");
		break;
	default:
		g_info("going %d", action);
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
