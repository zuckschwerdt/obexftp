#ifndef OBEXFTP_IO_H
#define OBEXFTP_IO_H

typedef enum {
	CD_CREATE=1,
	CD_ALLOWABS=2
} cd_flags;

obex_object_t *build_object_from_file(obex_t *handle, const char *localname, const char *remotename);
int open_safe(const char *path, const char *name);
int checkdir(const char *path, const char *dir, cd_flags flags);

#endif /* OBEXFTP_IO_H */
