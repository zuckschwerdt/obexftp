#ifndef IO_H
#define IO_H

#include <glib.h>

typedef enum {
	CD_CREATE=1,
	CD_ALLOWABS=2
} cd_flags;

obex_object_t *build_object_from_file(obex_t *handle, const gchar *localname, const gchar *remotename);
gint open_safe(const gchar *path, const gchar *name);
gint checkdir(const gchar *path, const gchar *dir, cd_flags flags);

#endif /* IO_H */
