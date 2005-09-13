#ifndef OBEXFTP_IO_H
#define OBEXFTP_IO_H

/*@null@*/ obex_object_t *build_object_from_file(obex_t *handle, uint32_t conn, const char *localname, const char *remotename);
int open_safe(const char *path, const char *name);
int checkdir(const char *path, const char *dir, int create, int allowabs);

#endif /* OBEXFTP_IO_H */
