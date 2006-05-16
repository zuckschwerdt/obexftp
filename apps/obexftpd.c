/*
 *  apps/obexftpd.c: OBEX server
 *
 *  Copyright (c) 2003 Christian W. Zuckschwerdt <zany@triq.net>
 *  Copyright (c) 2006 Alan Zhang <vibra@tom.com>
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
/*
 * Created at:    Don, 2 Okt 2003
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock.h>
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#endif /*_WIN32*/

/* just until there is a server layer in obexftp */
#include <openobex/obex.h>

#include <obexftp/obexftp.h>
#include <obexftp/client.h>
#include <obexftp/object.h>
#include <multicobex/multi_cobex.h>
#include <common.h>

#include "obexftp_sdp.h"

/* define this to "", "\r\n" or "\n" */
#define EOLCHARS "\n"

#if !defined(MAX)
#define MAX(a, b) \
	(a > b ? a : b)
#endif

/* Application defined headers */
#define HDR_CREATOR  0xcf	/* so we don't require OpenOBEX 1.3 */

#define WORK_PATH_MAX		128
#define CUR_DIR				"./"

//BEGIN of constant

bdaddr_t *bt_src = NULL;
char *device = NULL;
int channel = 10; /* OBEX_PUSH_HANDLE */

//const char *root_path = "/root/obexFTP/obexftp-0.10.7/apps/";
char init_work_path[WORK_PATH_MAX];

static char fl_type[] = XOBEX_LISTING;

/* current command, set by main, read from info_cb */
int c;

volatile int finished = 0;
volatile int success = 0;
volatile int obexftpd_reset = 0;

int connection_id = 0;
char * filename_of_put = NULL;
//END of constant

static in_addr_t inaddr_any = INADDR_ANY;
static int parsehostport(const char *name, char **host, int *port) {
	struct hostent *e;
	char *p, *s;
	uint8_t	n[4];

	p = strdup(name);
		
	*port = 650;
	if ((s=strchr(p, ':'))) {
		*s = '\0';
		*port = atoi(++s);
	}

	inaddr_any = INADDR_ANY;
	*host = (char *)&inaddr_any;

	if (sscanf(p, "%d.%d.%d.%d", &n[0], &n[1], &n[2], &n[3]) == 4) {
		inaddr_any = (in_addr_t) (*n);
	} else {
		e = gethostbyname(p);
		if (e) {
			*host = e->h_addr_list[0];
		}
	}
	free(p);
	return 0;
}


//BEGIN of compositor the folder listing XML document
typedef struct 
{
	char 			*data;
	unsigned int	block_size;
	unsigned int 	size;
	unsigned int 	max_size;
} rawdata_stream_t;

static rawdata_stream_t* INIT_RAWDATA_STREAM(unsigned int size)
{
	rawdata_stream_t *stream = (rawdata_stream_t *)malloc(sizeof(rawdata_stream_t));
	if (NULL == stream)
	{
		return NULL;
	}
	else
	{
		stream->data = (char *)malloc(size);
		if (NULL == stream->data)
		{
			free(stream);
			return NULL;
		}
		else
		{
			strcpy(stream->data, "");
			stream->size = 0;
			stream->block_size = size;
			stream->max_size = size;
		}
	}

	return stream;
}

static int ADD_RAWDATA_STREAM_DATA(rawdata_stream_t *stream, char *data)
{
	char 			*databuf = NULL;
	unsigned int 	size, new_size;

	size = strlen(data);
	if ((size + stream->size) >= stream->max_size)
	{
		databuf = stream->data;
		//printf("b malloc: stream->data=%x, databuf=%x\n", stream->data, databuf);
		//printf("data: %s\n", stream->data);
		//printf("size=%d, max_size=%d\n", stream->size, stream->max_size);
		new_size = MAX(size + stream->size, stream->max_size + stream->block_size);
		//printf("new size=%d\n", new_size);
		while(stream->max_size <= new_size)
		{
			stream->max_size += stream->block_size;
		}
		//printf("stream->max_size=%d\n", stream->max_size);	
		stream->data = (char *)realloc(stream->data, stream->max_size);
		//printf("a malloc: stream->data=%x, databuf=%x\n", stream->data, databuf);
		if (NULL == stream->data)
		{
			printf("realloc return NULL!!!\n");
			stream->data = databuf;
			return 0; 
		}
		else
		{
			strcat(stream->data, data);
			stream->size += size;
			//printf("size=%d, max_size=%d\n", stream->size, stream->max_size);
		}
	}
	else
	{
		//printf("b malloc: stream->data=%x, databuf=%x\n", stream->data, databuf);
		strcat(stream->data, data);
		stream->size += size;
	}
	return 1;
}

static void FREE_RAWDATA_STREAM(rawdata_stream_t *stream)
{
	free(stream->data);
	free(stream);
}

inline static void FL_XML_HEADER_BEGIN(rawdata_stream_t *stream)
{
	//NULL
}
	
inline static void FL_XML_VERSION(rawdata_stream_t *stream)	
{
	ADD_RAWDATA_STREAM_DATA(stream, "<?xml version=\"1.0\"?>" EOLCHARS);
}

inline static void FL_XML_TYPE(rawdata_stream_t *stream) 
{
	ADD_RAWDATA_STREAM_DATA(stream, "<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">" EOLCHARS);
}

inline static void FL_XML_HEADER_END(rawdata_stream_t *stream)
{
	//NULL
}

inline static void FL_XML_BODY_BEGIN(rawdata_stream_t *stream)	
{
	ADD_RAWDATA_STREAM_DATA(stream, "<folder-listing version=\"1.0\">" EOLCHARS);
}

inline static void FL_XML_BODY_END(rawdata_stream_t *stream)	
{
	ADD_RAWDATA_STREAM_DATA(stream, "</folder-listing>" EOLCHARS);
}

inline static void FL_XML_BODY_ITEM_BEGIN(rawdata_stream_t *stream)	
{
	ADD_RAWDATA_STREAM_DATA(stream, "<");
}

inline static void FL_XML_BODY_ITEM_END(rawdata_stream_t *stream)	
{
	ADD_RAWDATA_STREAM_DATA(stream, "/>" EOLCHARS);
}

inline static void FL_XML_BODY_FOLDERNAME(rawdata_stream_t *stream, char *name)
{
	ADD_RAWDATA_STREAM_DATA(stream, "folder name=\""); 
	ADD_RAWDATA_STREAM_DATA(stream, name);		
	ADD_RAWDATA_STREAM_DATA(stream, "\" ");
}

inline static void FL_XML_BODY_FILENAME(rawdata_stream_t *stream, char *name)
{
	ADD_RAWDATA_STREAM_DATA(stream, "file name=\""); 
	ADD_RAWDATA_STREAM_DATA(stream, name);		
	ADD_RAWDATA_STREAM_DATA(stream, "\" ");
}

inline static void FL_XML_BODY_SIZE(rawdata_stream_t *stream, unsigned int size)	
{
	char str_size[16];

	ADD_RAWDATA_STREAM_DATA(stream, "size=\"");
	sprintf(str_size, "%d", size);
	ADD_RAWDATA_STREAM_DATA(stream, str_size);
	ADD_RAWDATA_STREAM_DATA(stream, "\" ");
}

inline static void FL_XML_BODY_PERM(rawdata_stream_t *stream)	
{
	ADD_RAWDATA_STREAM_DATA(stream, "user-perm=\"RWD\" ");
}

inline static void FL_XML_BODY_MTIME(rawdata_stream_t *stream, time_t time)	
{
	struct tm 	tm;
	char		str_tm[16];
	
	ADD_RAWDATA_STREAM_DATA(stream, "modified=\"");
	tm = *localtime(&time);
	sprintf(str_tm, "%d%02d%02dT%02d%02d%02dZ", 
		1900 + tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	ADD_RAWDATA_STREAM_DATA(stream, str_tm);	
	ADD_RAWDATA_STREAM_DATA(stream, "\" ");
}

inline static void FL_XML_BODY_CTIME(rawdata_stream_t *stream, time_t time)	
{
	struct tm 	tm;
	char		str_tm[16];
	
	ADD_RAWDATA_STREAM_DATA(stream, "created=\"");
	tm = *localtime(&time);
	sprintf(str_tm, "%d%02d%02dT%02d%02d%02dZ", 
		1900 + tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	ADD_RAWDATA_STREAM_DATA(stream, str_tm);	
	ADD_RAWDATA_STREAM_DATA(stream, "\" ");
}

inline static void FL_XML_BODY_ATIME(rawdata_stream_t *stream, time_t time)
{
	struct tm 	tm;
	char		str_tm[16];
	
	ADD_RAWDATA_STREAM_DATA(stream, "accessed=\"");
	tm = *localtime(&time);
	sprintf(str_tm, "%d%02d%02dT%02d%02d%02dZ", 
		1900 + tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	ADD_RAWDATA_STREAM_DATA(stream, str_tm);	
	ADD_RAWDATA_STREAM_DATA(stream, "\" ");
}

//END of compositor the folder listing XML document

inline static int is_type_fl(char *type)
{
	if (NULL == type)
	{
		return 0;
	}
	
	if (0 == strcmp(type, fl_type))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void connect_server(obex_t *handle, obex_object_t *object)
{
	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;

	const uint8_t *who = NULL;
	int who_len = 0;
	printf("%s()\n", __FUNCTION__);

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		if(hi == OBEX_HDR_WHO)	{
			who = hv.bs;
			who_len = hlen;
		}
		else	{
			printf("%s() Skipped header %02x\n", __FUNCTION__, hi);
		}
	}
	if (who_len == 6)	{
		if(strncmp("Linux", who, 6) == 0)	{
			printf("Weeeha. I'm talking to the coolest OS ever!\n");
		}
	}
	OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
	if(OBEX_ObjectAddHeader(handle, object, OBEX_HDR_CONNECTION,
              		(obex_headerdata_t)((uint32_t) connection_id),
                      	sizeof(uint32_t),
                            OBEX_FL_FIT_ONE_PACKET) < 0 )    {
                DEBUG(1, "Error adding header\n");
                OBEX_ObjectDelete(handle, object);
                return;
        } 
}


static void set_server_path(obex_t *handle, obex_object_t *object)
{
	char *name = NULL;
	char fullname[WORK_PATH_MAX];

	// "Backup Level" and "Don't Create" flag in first byte
	uint8_t setpath_nohdr_dummy = 0;
	uint8_t *setpath_nohdr_data;
	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;

	OBEX_ObjectGetNonHdrData(object, &setpath_nohdr_data);
	if (NULL == setpath_nohdr_data) {
		setpath_nohdr_data = &setpath_nohdr_dummy;
		printf("nohdr data not found\n");
	}
	printf("nohdr data: %x\n", *setpath_nohdr_data);

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			printf("%s() Found name\n", __FUNCTION__);
			if (0 < hlen)
			{
				if( (name = malloc(hlen / 2)))	{
					OBEX_UnicodeToChar(name, hv.bs, hlen);
					printf("name:%s\n", name);
				}
			}
			else if (0 == hlen)
			{
				printf("set path to root\n");
				chdir(init_work_path);
			}
			else
			{
				printf("hlen < 0, impossible!\n");
			}
			break;
			
		default:
			printf("%s() Skipped header %02x\n", __FUNCTION__, hi);
		}
	}	
	
	if (name)
	{
		strcpy(fullname, CUR_DIR);
		strcat(fullname, name);
		if ((*setpath_nohdr_data & 2) == 0) {
			printf("mkdir %s\n", name);
			if (0 > mkdir(fullname, 0755)) {
				perror("requested mkdir failed");
			}
		}
		if (0 > chdir(fullname))
		{
			OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_FORBIDDEN);
		}
		
		free(name);
		name = NULL;
	}
}

//
// Get the filesize in a "portable" way
//
static int get_filesize(const char *filename)
{
#ifdef _WIN32
	HANDLE fh;
	int size;
	fh = CreateFile(filename, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(fh == INVALID_HANDLE_VALUE) {
		printf("Cannot open %s\n", filename);
		return -1;	
	}
	size = GetFileSize(fh, NULL);
	printf("fize size was %d\n", size);
	CloseHandle(fh);
	return size;

#else
	struct stat stats;
	/*  Need to know the file length */
	stat(filename, &stats);
	return (int) stats.st_size;
#endif
}

//
// Read a file and alloc a buffer for it
//
static uint8_t* easy_readfile(const char *filename, int *file_size)
{
	int actual;
	int fd;
	uint8_t *buf;

	*file_size = get_filesize(filename);
	printf("name=%s, size=%d\n", filename, *file_size);

#ifdef _WIN32
	fd = open(filename, O_RDONLY | O_BINARY, 0);
#else
	fd = open(filename, O_RDONLY, 0);
#endif

	if (fd == -1) {
		return NULL;
	}
	
	if(! (buf = malloc(*file_size)) )	{
		return NULL;
	}

	actual = read(fd, buf, *file_size);
	close(fd); 

#ifdef _WIN32
	if(actual != *file_size) {
		free(buf);
		buf = NULL;
	}
#else
	*file_size = actual;
#endif
	return buf;
}

static void get_server(obex_t *handle, obex_object_t *object)
{
	uint8_t *buf = NULL;

	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;
	int file_size;

	char *name = NULL;
	char *type = NULL;

	printf("%s()\n", __FUNCTION__);

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			printf("%s() Found name\n", __FUNCTION__);
			if( (name = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(name, hv.bs, hlen);
				printf("name:%s\n", name);
			}
			break;
			
		case OBEX_HDR_TYPE:
			if( (type = malloc(hlen + 1)))	{
				strcpy(type, hv.bs);
			}
			printf("%s() type:%s\n", __FUNCTION__, type);

		case 0xbe: // user-defined inverse push
			printf("%s() Found inverse push req\n", __FUNCTION__);
       			printf("data:%02x\n", hv.bq1);
			break;
			

		case OBEX_HDR_APPARAM:
			printf("%s() Found apparam\n", __FUNCTION__);
       			printf("name:%d (%02x %02x ...)\n", hlen, hv.bs, hv.bs+1);
			break;
			
		default:
			printf("%s() Skipped header %02x\n", __FUNCTION__, hi);
		}
	}
fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
	if (is_type_fl(type))
	{
		struct dirent 		*dirp;
		DIR 				*dp;
		struct stat 		statbuf;
		char				*filename;
		rawdata_stream_t 	*xmldata;

		xmldata = INIT_RAWDATA_STREAM(512);
		if (NULL == xmldata)
		{
			goto out;
		}
		FL_XML_HEADER_BEGIN(xmldata);
		FL_XML_VERSION(xmldata);
		FL_XML_TYPE(xmldata);
		FL_XML_HEADER_END(xmldata);
		FL_XML_BODY_BEGIN(xmldata);

		dp = opendir(CUR_DIR);
		while(NULL != dp && NULL != (dirp = readdir(dp))) 
		{
			if (0 == strcmp(dirp->d_name, ".") || 0 == strcmp(dirp->d_name, ".."))
			{
				continue;
			}

			FL_XML_BODY_ITEM_BEGIN(xmldata);

			//Adding 1 bytes due to containing '\0' 
			filename = malloc(strlen(CUR_DIR) + strlen(dirp->d_name) + 1);
			strcpy(filename, CUR_DIR);
			strcat(filename, dirp->d_name);

			lstat(filename, &statbuf);
			if (0 == S_ISDIR(statbuf.st_mode)) //it is a file
			{
				FL_XML_BODY_FILENAME(xmldata, dirp->d_name);				
			}
			else	//it is a directory
			{
				FL_XML_BODY_FOLDERNAME(xmldata, dirp->d_name);
			}
			
			FL_XML_BODY_SIZE(xmldata, statbuf.st_size);
			FL_XML_BODY_PERM(xmldata);
			FL_XML_BODY_MTIME(xmldata, statbuf.st_mtime);
			FL_XML_BODY_CTIME(xmldata, statbuf.st_ctime);
			FL_XML_BODY_ATIME(xmldata, statbuf.st_atime);

			FL_XML_BODY_ITEM_END(xmldata);
			
			//printf("---filename:%s\n", filename);
			fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
			free(filename);
			filename = NULL;
			fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
		}
fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
		FL_XML_BODY_END(xmldata);

		closedir(dp);
		printf("xml doc:%s\n", xmldata->data);
		
		//composite the obex obejct
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		hv.bs = xmldata->data;
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hv, xmldata->size, 0);
		hv.bq4 = xmldata->size;
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);
		fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
		FREE_RAWDATA_STREAM(xmldata);
		fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
	}
	else if (name)
	{
		printf("%s() Got a request for %s\n", __FUNCTION__, name);
		
		buf = easy_readfile(name, &file_size);
		if(buf == NULL) {
			printf("Can't find file %s\n", name);
			OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
			return;
		}

		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		hv.bs = buf;
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY, hv, file_size, 0);
		hv.bq4 = file_size;
		OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hv, sizeof(uint32_t), 0);
	}
	else
	{
		printf("%s() Got a GET without a name-header!\n", __FUNCTION__);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_FOUND, OBEX_RSP_NOT_FOUND);
		return;
	}
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
	if (NULL != buf)
	{
		free(buf);
	}
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
out:
	if (NULL != name)
	{
		free(name);
	}
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
	if (NULL != type)
	{
		free(type);
	}
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
	
	return;
}

/*
 * Function safe_save_file ()
 *
 *    First remove path and add "/tmp/". Then save.
 *
 */
static int safe_save_file(char *name, const uint8_t *buf, int len)
{
	char *s = NULL;
	char filename[255] = {0,};
	int fd;
	int actual;


	printf("Filename = %s\n", name);

#ifndef _WIN32
	sprintf( filename, CUR_DIR);
#endif
	s = strrchr(name, '/');
	if (s == NULL)
		s = name;
	else
		s++;

	strncat(filename, s, 250);
#ifdef _WIN32
	fd = open(filename, O_RDWR | O_CREAT, 0);
#else
	fd = open(filename, O_RDWR | O_CREAT, DEFFILEMODE);
#endif

	if ( fd < 0) {
		perror( filename);
		return -1;
	}
	
	actual = write(fd, buf, len);
	close(fd);

	printf( "Wrote %s (%d bytes)\n", filename, actual);

	return actual;
}


/*
 * Function put_done()
 *
 *    Parse what we got from a PUT
 *
 */
static void put_done(obex_t *handle, obex_object_t *object, int final)
{
	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;

	const uint8_t *body = NULL;
	int body_len = 0;
	static char *name = NULL;
	char fullname[WORK_PATH_MAX];
	struct stat statbuf;
	//char *namebuf = NULL;

	fprintf(stderr, "put_done>>>\n");
	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_BODY:
			body = hv.bs;
			body_len = hlen;
			fprintf(stderr, "hv.bs=%p, hlen=%d\n", hv.bs, hlen);
			break;
		case OBEX_HDR_NAME:
			if (NULL != name)
			{
				free(name);
			}
			if( (name = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(name, hv.bs, hlen);
				fprintf(stderr, "put file name: %s\n", name);
			}
			break;

		case OBEX_HDR_LENGTH:
			printf("HEADER_LENGTH = %d\n", hv.bq4);
			break;

		case HDR_CREATOR:
			printf("CREATORID = %#x\n", hv.bq4);
			break;
		
		default:
			printf("%s () Skipped header %02x\n", __FUNCTION__ , hi);
		}
	}
	if(!body)	{
		printf("Got a PUT without a body\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
	}
	if(!name)	{
		name = strdup("OBEX_PUT_Unknown_object");
		printf("Got a PUT without a name. Setting name to %s\n", name);

	}
	if (body)
	{
		safe_save_file(name, body, body_len);
	}
	if(final && !body) {
		strcpy(fullname, CUR_DIR);
		strcat(fullname, name);
		if (!stat(fullname, &statbuf)) {
			perror("stat failed");
		}
		if (S_ISDIR(statbuf.st_mode)) {
			printf("Removing dir %s\n", name);
			rmdir(fullname);
		} else {
			printf("Deleting file %s\n", name);
			unlink(fullname);
		}
	}
	if (final)
	{
		free(name);
		name = NULL;
		fprintf(stderr, "<<<put_done\n");
	}
}


/*
 * Function server_indication()
 *
 * Called when a request is about to come or has come.
 *
 */
static void server_request(obex_t *handle, obex_object_t *object, int event, int cmd)
{
	switch(cmd)	{
	case OBEX_CMD_SETPATH:
		printf("Received SETPATH command\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		set_server_path(handle, object);
		break;
	case OBEX_CMD_GET:
		/* A Get always fits one package */
		get_server(handle, object);
		break;
	case OBEX_CMD_PUT:
		printf("Received PUT command\n");
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		put_done(handle, object, 1);
		break;
	case OBEX_CMD_CONNECT:
//		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		connect_server(handle, object);
		connection_id++;
		break;
	case OBEX_CMD_DISCONNECT:
		OBEX_ObjectSetRsp(object, OBEX_RSP_SUCCESS, OBEX_RSP_SUCCESS);
		break;
	default:
		printf("%s () Denied %02x request\n", __FUNCTION__, cmd);
		OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
		break;
	}
	return;
}


static void obex_event(obex_t *handle, obex_object_t *obj, int mode, int event, int obex_cmd, int obex_rsp)
{
	char progress[] = "\\|/-";
	static unsigned int i = 0;

	switch (event) {
	case OBEX_EV_STREAMAVAIL:
       	printf("Time to read some data from stream\n");
        break;

	case OBEX_EV_LINKERR:
        finished = 1;
        obexftpd_reset = 1;
        success = FALSE;
		fprintf(stderr, "failed: %d\n", obex_cmd);
		break;

    	case OBEX_EV_REQ:
        printf("Incoming request %02x\n", obex_cmd);
		/* Comes when a server-request has been received. */
		server_request(handle, obj, event, obex_cmd);
		break;
		
	case OBEX_EV_REQHINT:
        /* An incoming request is about to come. Accept it! */
		switch(obex_cmd) {
		case OBEX_CMD_PUT:
		case OBEX_CMD_CONNECT:
		case OBEX_CMD_DISCONNECT:
			OBEX_ObjectSetRsp(obj, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			break;
		default:
			OBEX_ObjectSetRsp(obj, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
			break;
		}
		break;

	case OBEX_EV_REQCHECK:
		/* e.g. mode=01, obex_cmd=03, obex_rsp=00 */
            	printf("%s() OBEX_EV_REQCHECK: mode=%02x, obex_cmd=%02x, obex_rsp=%02x\n", __func__, 
				mode, obex_cmd, obex_rsp);
		OBEX_ObjectSetRsp(obj, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;
	case OBEX_EV_REQDONE:
	        //finished = TRUE;
        	if(obex_rsp == OBEX_RSP_SUCCESS)
	        	success = TRUE;
        	else {
	            success = FALSE;
        	    printf("%s() OBEX_EV_REQDONE: obex_rsp=%02x\n", __func__, obex_rsp);
	        }
		break;

	case OBEX_EV_PROGRESS:
		fprintf(stderr, "%c%c", 0x08, progress[i++]);
		fflush(stdout);
		if (i >= strlen(progress))
			i = 0;
			
		switch(obex_cmd) {
		case OBEX_CMD_PUT:
			fprintf(stderr, "obex_ev_progress: obex_cmd_put\n");
			put_done(handle, obj, 0);
			break;
		default:
			break;
		}
		break;

	case OBEX_EV_STREAMEMPTY:
		//(void) cli_fillstream(cli, object);
		break;

    default:
         printf("%s() Unhandled event %d\n", __func__, event);
         break;

	}
}


static void start_server(int transport)
{
	int use_sdp = 0;
	obex_t *handle = NULL;
	struct sockaddr_in saddr;

	if (NULL == getcwd(init_work_path, WORK_PATH_MAX) && ERANGE == errno)
	{
		fprintf(stderr, "Wah, work path is too long, exceed %d.\n", WORK_PATH_MAX);
		exit(0);
	}

       	if (transport==OBEX_TRANS_BLUETOOTH && 0 > obexftp_sdp_register())
       	{
       		//OBEX_Cleanup(handle);
       		fprintf(stderr, "register to SDP Server failed.\n");
       		exit(0);
       	}
       	else
       	{
       		use_sdp = 1;
       	}
	
reset:
	handle = OBEX_Init(transport, obex_event, 0);
	if (NULL == handle) {
       		perror("failed to init obex.");
       		exit(-1);
	}

	switch (transport) {
       	case OBEX_TRANS_INET:
	        saddr.sin_family = AF_INET;
	        saddr.sin_port = htons(channel);
	        saddr.sin_addr.s_addr = (in_addr_t)*device; //INADDR_ANY;
		//InOBEX_ServerRegister(handle); /* always port 650 */
		if (0 > OBEX_ServerRegister(handle, (struct sockaddr *)&saddr, sizeof(saddr))) {
       			perror("failed to register inet server");
	       		exit(-1);
		}
       	break;
       	case OBEX_TRANS_BLUETOOTH:
		if (0 > BtOBEX_ServerRegister(handle, bt_src, channel)) {
       			perror("failed to register bluetooth server");
	       		exit(-1);
		}
       	break;
       	case OBEX_TRANS_IRDA:
		if (0 > IrOBEX_ServerRegister(handle, "")) {
       			perror("failed to register IrDA server");
	       		exit(-1);
		}
       	break;
       	case OBEX_TRANS_CUSTOM:
		/* A simple Ericsson protocol session perhaps? */
       	default:
       		fprintf(stderr, "Transport type unknown\n");
	       		exit(-1);
	}
	printf("Waiting for connection...\n");

	(void) OBEX_ServerAccept(handle, obex_event, NULL);

	while (!finished) {
		//printf("Handling connection...\n");
		OBEX_HandleInput(handle, 1);
	}

	OBEX_Cleanup(handle);
	sleep(1); /* throttle */

	if (obexftpd_reset)
	{
		fprintf(stderr, "obexftpd reset\n");
		obexftpd_reset = 0;
		finished = 0;
		success = 0;
		goto reset;
	}
	
	if (use_sdp)
	{
		fprintf(stderr, "sdp unregister\n");
		obexftp_sdp_unregister();
	}

}

int main(int argc, char *argv[])
{
	int verbose = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"irda",	no_argument, NULL, 'i'},
			{"bluetooth",	no_argument, NULL, 'b'},
			{"tty",		required_argument, NULL, 't'},
			{"network",	required_argument, NULL, 'n'},
			{"chdir",	required_argument, NULL, 'c'},
			{"verbose",	no_argument, NULL, 'v'},
			{"version",	no_argument, NULL, 'V'},
			{"help",	no_argument, NULL, 'h'},
			{"usage",	no_argument, NULL, 'h'},
			{0, 0, 0, 0}
		};
		
		c = getopt_long (argc, argv, "-ibt:n:c:vVh",
				 long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		
		case 'i':
			start_server(OBEX_TRANS_IRDA);
			break;
		
		case 'b':
			start_server(OBEX_TRANS_BLUETOOTH);
			fprintf(stderr, "server end\n");
			break;
		
		case 'n':
			parsehostport(optarg, &device, &channel);
			//channel = atoi(optarg);
			start_server(OBEX_TRANS_INET);
			fprintf(stderr, "server end\n");
			break;
		
		case 't':
			printf("accepting on tty not implemented yet.");
			/* start_server(OBEX_TRANS_CUSTOM); optarg */
			break;

		case 'c':
			chdir(optarg);
			break;

		case 'v':
			verbose++;
			break;

		case 'V':
			printf("ObexFTPd %s\n", VERSION);
			break;

		case 'h':
			printf("ObexFTPd %s\n", VERSION);
			printf("Usage: %s  [-c <path>]  [-v]  [-i | -b | -t <dev> | -n <port>]\n"
				"Recieve files from/to Mobile Equipment.\n"
				"Copyright (c) 2003-2006 Christian W. Zuckschwerdt and Alan Zhang\n"
				"\n"
				" -i, --irda                  accept IrDA connections\n"
				" -b, --bluetooth             accept bluetooth connections\n"
				" -t, --tty <device>          accept connections from this tty\n"
				" -n, --network <port>        accept network connections\n\n"
				" -c, --chdir <path>          set a default basedir\n"
				" -v, --verbose               verbose messages\n\n"
				" -V, --version               print version info\n"
				" -h, --help, --usage         this help text\n"
				"\n"
				"THIS IS JUST AN EXAMPLE AND HAS NOT BEEN TESTED YET!\n"
				"\n",
				argv[0]);
			exit(0);

		default:
			printf("Try `%s --help' for more information.\n",
				 argv[0]);
		}
	}

	if (optind < argc) {
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
		exit (-1);
	}

	exit (0);

}
