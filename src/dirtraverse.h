#ifndef DIRTRAVERSE_H
#define DIRTRAVERSE_H

#include <glib.h>

typedef gint (*visit_cb)(gint action, gchar *name, gchar *path, gpointer userdata);
#define VISIT_FILE 1
#define VISIT_GOING_DEEPER 2
#define VISIT_GOING_UP 3

gint visit_all_files(gchar *name, visit_cb cb, gpointer userdata);

#endif
