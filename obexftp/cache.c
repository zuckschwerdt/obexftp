/*
 *  obexftp/cache.c: ObexFTP client library
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
#include <unistd.h>

#include <openobex/obex.h>

#include "obexftp.h"
#include "client.h"
#include "object.h"
#include "cache.h"

#include <common.h>


/*
 * Normalize the path argument, add/remove leading/trailing slash
 * turns relative paths into (most likely wrong) absolute ones
 * wont expand "../" or "./"
 */
static /*@only@*/ char *normalize_dir_path(int quirks, const char *name)
{
	char *copy, *p;

	return_val_if_fail(name != NULL, NULL);

	p = copy = malloc(strlen(name) + 2); /* at most add two slashes */

	if (OBEXFTP_USE_LEADING_SLASH(quirks))
		*p++ = '/';
	while (*name == '/') name++;

	while (*name) {
		if (*name == '/') {
			*p++ = *name++;
			while (*name == '/') name++;
		} else {
			*p++ = *name++;
		}
	}
	if (p > copy && *(p-1) == '/')
		p--;

	if (OBEXFTP_USE_TRAILING_SLASH(quirks))
		*p++ = '/';

	*p = '\0';

	return copy;
}


/*
 * methods that need to invalidate cache lines:
 * setpath (when create is on)
 * put
 * put_file
 * del
 * rename
 */
void cache_purge(cache_object_t **root, const char *path)
{
	cache_object_t *cache, *tmp;
	char *name;
	char *pathonly;

#define free_node(node) { \
                        if (node->name) \
				free(node->name); \
			if (node->content); \
				free(node->content); \
			free(node); \
			}

        if (!path || *path == '\0' || *path != '/') {
		/* purge all */
		while (*root) {
			cache = *root;
			*root = cache->next;
                        free_node(cache);
		}
		return;
	}
	
	pathonly = strdup(path);
	name = strrchr(pathonly, '/');
	*name++ = '\0';

        /* removing far too much, the siblings could stay cached... */
       	while (*root && !strncmp((*root)->name, pathonly, strlen(pathonly))) {
       		cache = *root;
       		*root = cache->next;
		free_node(cache);
       	}
	for (cache = *root; cache->next; cache = cache->next) {
		if (!strncmp(cache->next->name, pathonly, strlen(pathonly))) {
			tmp = cache->next;
			cache->next = cache->next->next;
			free_node(tmp);
		}
	}

	free(pathonly);
}

int get_cache_object(obexftp_client_t *cli, const char *name, char **object, int *size)
{
	cache_object_t *cache;
	return_val_if_fail(cli != NULL, -1);

	/* search the cache */
	for (cache = cli->cache; cache && strcmp(cache->name, name); cache = cache->next);
	if (cache) {
		DEBUG(2, "%s() Listing %s from cache\n", __func__, cache->name);
		if (object)
			*object = cache->content;
		if (size)
			*size = cache->size;
		return 0;
	}

	return -1;
}

int put_cache_object(obexftp_client_t *cli, const char *name, const char *object, int size)
{
	cache_object_t *cache;
	return_val_if_fail(cli != NULL, -1);

	/* prepend to cache */
	cache = cli->cache;
	cli->cache = calloc(1, sizeof(cache_object_t));
	cli->cache->next = cache;
	cli->cache->timestamp = time(NULL);
	cli->cache->size = size;
	cli->cache->name = name;
	cli->cache->content = object;

	return 0;
}

/* List a directory from cache, optionally loading it first. */
static char *obexftp_cache_list(obexftp_client_t *cli, const char *name)
{
	char *path, *listing;

	return_val_if_fail(cli != NULL, NULL);

	cli->infocb(OBEXFTP_EV_RECEIVING, name, 0, cli->infocb_data);

	path = normalize_dir_path(cli->quirks, name);
	DEBUG(2, "%s() Listing %s (%s)\n", __func__, name, path);

	/* search the cache */
	if (!get_cache_object(cli, path, &listing, NULL)) {
		DEBUG(2, "%s() Listing %s from cache\n", __func__, path);
		free(path);
		return listing;
	}

	if (!strcmp(path, "/telecom/")) {
		listing = strdup("<file name=\"devinfo.txt\">");
		put_cache_object(cli, path, listing, strlen(listing));
	}

	if (obexftp_list(cli, NULL, path) < 0)
		return NULL;
	listing = strdup(cli->buf_data);

	put_cache_object(cli, path, listing, strlen(listing));

	return listing;
}


/* simple xml parser */

static time_t atotime (const char *date)
{
	struct tm tm;

	if (6 == sscanf(date, "%4d%2d%2dT%2d%2d%2d",
			&tm.tm_year, &tm.tm_mday, &tm.tm_mon,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec)) {
		tm.tm_year -= 1900;
		tm.tm_mon--;
	}
	tm.tm_isdst = 0;

	return mktime(&tm);
}

/* very limited - not multi-byte character save */
static stat_entry_t *parse_directory(const char *xml)
{
        const char *line;
        const char *p;
        char name[201]; // bad coder
        char mod[201]; // - no biscuits!
        char size[201]; // int would be ok too.

	stat_entry_t *dir_start, *dir;
	int i;

	if (!xml)
		return NULL;

	/* prepare a cache to hold this dir */
	p = xml;
	for (i = 0; p && *p; p = strchr(++p, '>')) i++;
	DEBUG(2, "max %d cache lines\n", i);
	dir_start = dir = calloc(i, sizeof(stat_entry_t));

        for (line = xml; *line != '\0'; ) {
		
		p = line;
                line = strchr(line, '>');
		if (!line)
			break;
		line++;
		/* can sscanf skip leading whitespace? */
		while (*p != '<') p++;

                if (2 == sscanf (p, "<folder name=\"%200[^\"]\" modified=\"%200[^\"]\"", name, mod)) {
                        dir->mode = S_IFDIR | 0755;
                        strcpy(dir->name, name);
			dir->mtime = atotime(mod);
                        dir->size = 0;
			dir++;
                }
                if (3 == sscanf (p, "<file name=\"%200[^\"]\" size=\"%200[^\"]\" modified=\"%200[^\"]\"", name, size, mod)) {
                        dir->mode = S_IFREG | 0644;
                        strcpy(dir->name, name);
			dir->mtime = atotime(mod);
			dir->size = 0;
			sscanf(size, "%i", &i);
			dir->size = i; /* int to off_t */
			dir++;
                }
                // handle hidden folder!

        }

	dir->name[0] = '\0';

        return dir_start;
}


/* directory handling */

typedef struct dir_stream dir_stream_t;
struct dir_stream {
	stat_entry_t *cur;
	/* stat_entry_t *head; -- so we can free this? */
};

void *obexftp_opendir(obexftp_client_t *cli, const char *name)
{
	cache_object_t *cache;
	dir_stream_t *stream;
	char *abs;

	/* fetch dir if needed */
	(void) obexftp_cache_list(cli, name);

	/* search the cache */
	abs = normalize_dir_path(cli->quirks, name);
	for (cache = cli->cache; cache && strcmp(cache->name, abs); cache = cache->next);
	free(abs);
	if (!cache)
		return NULL;
	DEBUG(2, "%s() dir prepared (%s)\n", __func__, cache->name);
		 
	/* read dir */
	if (!cache->stats)
		cache->stats = parse_directory(cache->content);
	DEBUG(2, "%s() got stats\n", __func__);
	stream = malloc(sizeof(dir_stream_t));
	stream->cur = cache->stats;

	return (void *)stream;
}

/* the stat entry is a cache object so we do nothing */
int obexftp_closedir(void *dir) {
	if (!dir)
		return -1;
	free (dir);
	return 0;
}

stat_entry_t *obexftp_readdir(void *dir) {
	dir_stream_t *stream;
	
	stream = (dir_stream_t *)dir;
	if (!stream || !(stream->cur))
		return NULL;

	if (!(*stream->cur->name))
		return NULL;

	return stream->cur++;
}
	 
stat_entry_t *obexftp_stat(obexftp_client_t *cli, const char *name)
{
	cache_object_t *cache;
	stat_entry_t *entry;
	char *path, *basename, *abs;

	return_val_if_fail(name != NULL, NULL);

	path = strdup(name);
	basename = strrchr(path, '/');
	if (basename)
		*basename++ = '\0';
	else
		basename = name;
	DEBUG(2, "%s() stating '%s' / '%s'\n", __func__, path, basename);

	/* fetch dir if needed */
	(void) obexftp_cache_list(cli, path);

	/* search the cache for the path */
	abs = normalize_dir_path(cli->quirks, path);
	for (cache = cli->cache; cache && strcmp(cache->name, abs); cache = cache->next);
	free(abs);
	if (!cache)
		return NULL;
	DEBUG(2, "%s() found '%s'\n", __func__, cache->name);
		 
	/* read dir */
	if (!cache->stats)
		cache->stats = parse_directory(cache->content);
	DEBUG(2, "%s() got dir '%s'\n", __func__, path);
	
	/* then lookup the basename */
	for (entry = cache->stats; entry && *entry->name && strcmp(entry->name, basename); entry++);
	if (!entry || !(*entry->name))
		return NULL;

	DEBUG(2, "%s() got stats\n", __func__);
	return entry;

	/*
	dev_t         st_dev;      / * device * /
	ino_t         st_ino;      / * inode * /
	mode_t        st_mode;     / * protection * /
	nlink_t       st_nlink;    / * number of hard links * /
	uid_t         st_uid;      / * user ID of owner * /
	gid_t         st_gid;      / * group ID of owner * /
	dev_t         st_rdev;     / * device type (if inode device) * /
	off_t         st_size;     / * total size, in bytes * /
	blksize_t     st_blksize;  / * blocksize for filesystem I/O * /
	blkcnt_t      st_blocks;   / * number of blocks allocated * /
	time_t        st_atime;    / * time of last access * /
	time_t        st_mtime;    / * time of last modification * /
	time_t        st_ctime;    / * time of last status change * /
	*/

}

