#ifndef DIRTRAVERSE_H
#define DIRTRAVERSE_H

typedef int (*visit_cb)(int action, const char *name, const char *path, void *userdata);
#define VISIT_FILE 1
#define VISIT_GOING_DEEPER 2
#define VISIT_GOING_UP 3

int visit_all_files(const char *name, visit_cb cb, void *userdata);

#endif
