/*
 */

#include <glib.h>

/* Read a BFB answer */
gint do_bfb_read(int fd, guint8 *buffer, gint length);

/* Send an BFB init command an check for a valid answer frame */
gboolean do_bfb_init(int fd);

/* Send an AT-command an expect 1 line back as answer */
gint do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen);

/* close the connection */
void bfb_io_close(int fd, int force);

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
gint bfb_io_open(const gchar *ttyname);

