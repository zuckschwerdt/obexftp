/*
 *  apps/discovery.c: Discover mobile and detect its class (Siemens, Ericsson)
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
/*
 *  v0.1:  Die, 30 Jul 2002 14:38:56 +0200
 */

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <g_debug.h>

struct mobile_info {
	gchar *gmi;	/* AT+GMI : SIEMENS */
	gchar *gmm;	/* AT+GMM : Gipsy Soft Protocolstack */
	gchar *gmr;	/* AT+GMR : V2.550 */
	gchar *cgmi;	/* AT+CGMI : SIEMENS */
	gchar *cgmm;	/* AT+CGMM : S45 */
	gchar *cgmr;	/* AT+CGMR : 21 */
	const gchar *ttyname;
	speed_t speed;
};

/* S45: (auto baud)
   SIEMENS, Gipsy Soft Protocolstack, V2.550
   SIEMENS, S45, 21
 */
/* S25: (fixed at 19200)
   SIEMENS, Gipsy Soft Protocolstack, V2.400
   SIEMENS, S25, 42
 */
/* T68:
   Ericsson, T68, R2E006      prgCXC125326_TAE
   ERICSSON, 1130202-BVT68, R2E006      CXC125319
 */

/* if auto-baud is enabled this will yoield the highest speed first */
speed_t speeds[] = {B115200, B57600, B38400, B19200, B9600, B0};

/* Send an AT-command an expect 1 line back as answer */
static gint do_at_cmd(int fd, char *cmd, char *rspbuf, int rspbuflen)
{
	fd_set ttyset;
	struct timeval tv;

	char *answer;
	char *answer_end = NULL;
	unsigned int answer_size;

	char tmpbuf[200] = {0,};
	int actual;
	int total = 0;
	int done = 0;
	int cmdlen;

        g_return_val_if_fail (fd > 0, -1);
        g_return_val_if_fail (cmd != NULL, -1);

	cmdlen = strlen(cmd);

	rspbuf[0] = 0;
	g_debug(G_GNUC_FUNCTION "() Sending %d: %s", cmdlen, cmd);

	// Write command
	if(write(fd, cmd, cmdlen) < cmdlen)	{
		g_warning("Error writing to port");
		return -1;
	}

	while(!done)	{
		FD_ZERO(&ttyset);
		FD_SET(fd, &ttyset);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		if(select(fd+1, &ttyset, NULL, NULL, &tv))	{
			sleep(1);
			actual = read(fd, &tmpbuf[total], sizeof(tmpbuf) - total);
			if(actual < 0)
				return actual;
			total += actual;

			g_debug(G_GNUC_FUNCTION "() tmpbuf=%d: %s", total, tmpbuf);

			// Answer not found within 100 bytes. Cancel
			if(total == sizeof(tmpbuf))
				return -1;

			if( (answer = index(tmpbuf, '\n')) )	{
				// Remove first line (echo)
				if( (answer_end = index(answer+1, '\n')) )	{
					// Found end of answer
					done = 1;
				}
			}
		}
		else	{
			// Anser didn't come in time. Cancel
			return -1;
		}
	}

//	g_debug(G_GNUC_FUNCTION "() buf:%08lx answer:%08lx end:%08lx", tmpbuf, answer, answer_end);


	g_debug(G_GNUC_FUNCTION "() Answer: %s", answer);

	// Remove heading and trailing \r
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer_end == '\r') || (*answer_end == '\n'))
		answer_end--;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	if((*answer == '\r') || (*answer == '\n'))
		answer++;
	g_debug(G_GNUC_FUNCTION "() Answer: %s", answer);

	answer_size = (answer_end) - answer +1;

	g_info(G_GNUC_FUNCTION "() Answer size=%d", answer_size);
	if( (answer_size) >= rspbuflen )
		return -1;


	strncpy(rspbuf, answer, answer_size);
	rspbuf[answer_size] = 0;
	return 0;
}

/* Init the phone and set it in BFB-mode */
/* Returns fd or -1 on failure */
struct mobile_info *probe_tty(const gchar *ttyname)
{
	gint ttyfd;
	gint speed;
	struct termios oldtio, newtio;
	guint8 rspbuf[200];
	struct mobile_info *info;
	gchar *p;

        g_return_val_if_fail (ttyname != NULL, NULL);

	g_debug(G_GNUC_FUNCTION "() ");

	info = calloc(1, sizeof(struct mobile_info));
	info->ttyname = ttyname;

	if( (ttyfd = open(ttyname, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) < 0 ) {
		g_error("Can' t open tty");
		free(info);
		return NULL;
	}

	/* probe mobile */
	tcgetattr(ttyfd, &oldtio);

	for (speed = 0; speeds[speed] != B0; speed++) {

		bzero(&newtio, sizeof(newtio));
		newtio.c_cflag = speeds[speed] | CS8 | CREAD;
		newtio.c_iflag = IGNPAR;
		newtio.c_oflag = 0;
		tcflush(ttyfd, TCIFLUSH);
		tcsetattr(ttyfd, TCSANOW, &newtio);

		/* is this ok? do we need to send both cr and lf? */
		if(do_at_cmd(ttyfd, "AT\r", rspbuf, sizeof(rspbuf)) < 0) {
			g_warning("Comm-error doing AT");
		}
		else if(strcasecmp("OK", rspbuf) == 0) {
			g_info("OKAY doing AT (%s)", rspbuf);
			break;
		}

	}
	if(speeds[speed] == B0) {
		close(ttyfd);
		free(info);
		g_warning("Nothing found");
		return NULL;
	}
	info->speed = speeds[speed];

	/* detect mobile */
	if(do_at_cmd(ttyfd, "AT+GMI\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		g_warning("Comm-error doing AT+GMI");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->gmi = g_strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+GMM\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		g_warning("Comm-error doing AT+GMM");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->gmm = g_strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+GMR\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		g_warning("Comm-error doing AT+GMR");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->gmr= g_strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+CGMI\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		g_warning("Comm-error doing AT+CGMI");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->cgmi = g_strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+CGMM\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		g_warning("Comm-error doing AT+CGMM");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->cgmm = g_strdup(rspbuf);

	if(do_at_cmd(ttyfd, "AT+CGMR\r", rspbuf, sizeof(rspbuf)) < 0) {
		close(ttyfd);
		free(info);
		g_warning("Comm-error doing AT+CGMR");
		return NULL;
	}
	if ((p=strchr(rspbuf, '\r')) != NULL) *p = 0;
	if ((p=strchr(rspbuf, '\n')) != NULL) *p = 0;
	info->cgmr= g_strdup(rspbuf);

	close(ttyfd);
	return info;

}

void g_log_null_handler (const gchar *log_domain,
                         GLogLevelFlags log_level,
                         const gchar *message,
                         gpointer user_data) {
}

int main(int argc, char *argv[])
{
	struct mobile_info *info;

        g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO,
                           g_log_null_handler, NULL);

	if (argc > 1)
		info = probe_tty(argv[1]);
	else
		info = probe_tty("/dev/ttyS0");

	if (info != NULL)
		g_print("%s (%d):\n%s, %s, %s\n%s, %s, %s\n",
			info->ttyname, info->speed,
			info->gmi, info->gmm, info->gmr,
			info->cgmi, info->cgmm, info->cgmr);

	return (EXIT_SUCCESS);
}
