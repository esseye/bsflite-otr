/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WATCOM_WIN32
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <limits.h>
#ifdef PLAN9
#include <time.h>
#endif

#ifdef __linux__
#include <linux/limits.h>
#endif

#ifdef __sun
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#endif

#ifdef PLAN9
#define PATH_MAX 1024		/* again, a guess */
#endif

char            logpath[PATH_MAX];
extern struct BuddyList *buddylist;
extern struct Conn *conn;
int             logging;

/* PROTO */
int
open_log_dir(void)
{
	DIR            *tmp;
#if !defined(__MINGW32__) && !defined(__DJGPP__)
	char           *homedir;
#endif

	logging = 1;

#if !defined(__MINGW32__) && !defined(__DJGPP__)

#ifdef PLAN9
	homedir = getenv("home");
	snprintf(logpath, sizeof(logpath), "%s/lib/bsflite", homedir);
	mkdir(logpath, 0777);
	snprintf(logpath, sizeof(logpath), "%s/lib/bsflite/log", homedir);
#else
	umask(077);

	homedir = getenv("HOME");
	snprintf(logpath, sizeof(logpath), "%s/.bsflite", homedir);
	mkdir(logpath, 0777);

	snprintf(logpath, sizeof(logpath), "%s/.bsflite/log", homedir);
#endif				/* PLAN9 */

#else
	snprintf(logpath, sizeof(logpath), "log");
#endif
	tmp = opendir(logpath);
	if (tmp == NULL) {
#ifndef __MINGW32__
		if (mkdir(logpath, 0777) == -1) {
#else
		if (mkdir(logpath) == -1) {
#endif
			perror("Couldn't make log directory: ");
			logging = 0;
			return -1;
		}
		return 0;
	} else
		closedir(tmp);

	return 0;
}

/* PROTO */
void
log_event(int event_type, char *sn, char *msg)
{
	FILE           *logfile;
	char            user_log[PATH_MAX];
	char            ts[21];
	struct tm      *now;
	time_t          t;

	if (!logging)
		return;

	if (conn->single_log)
		snprintf(user_log, sizeof(user_log), "%s/log", logpath);
	else
		snprintf(user_log, sizeof(user_log), "%s/%s.log", logpath, sn);


	logfile = fopen(user_log, "a");
	if (logfile == NULL)
		return;

	t = time(0);
	now = localtime(&t);
	strftime(ts, 20, "%m/%d/%Y %H:%M:%S", now);

	switch (event_type) {
	case EVENT_IM:
		if (conn->single_log)
			fprintf(logfile, "%s: %s: %s\n", ts, sn, msg);
		else
			fprintf(logfile, "%s: <- %s\n", ts, msg);
		break;
	case EVENT_IM_AUTORESPONSE:
		if (conn->single_log)
			fprintf(logfile, "%s: %s: *AUTO* %s\n", ts, sn, msg);
		else
			fprintf(logfile, "%s: <- *AUTO* %s\n", ts, msg);
		break;
	case EVENT_SIGNON:
		if (conn->single_log)
			fprintf(logfile, "%s: %s signed on.\n", ts, sn);
		else
			fprintf(logfile, "%s: Buddy signed on.\n", ts);
		break;
	case EVENT_SIGNOFF:
		if (conn->single_log)
			fprintf(logfile, "%s: %s signed off.\n", ts, sn);
		else
			fprintf(logfile, "%s: Buddy signed off.\n", ts);
		break;
	case EVENT_IMSEND:
		if (conn->single_log)
			fprintf(logfile, "%s: >%s: %s\n", ts, sn, msg);
		else
			fprintf(logfile, "%s: -> %s\n", ts, msg);
		break;
	}

	fclose(logfile);
	return;
}

/* PROTO */
void
show_log(int lines, char *sn)
{
	FILE           *logfile;
	char            user_log[PATH_MAX];
	char            buf[1024], *sp;
	char            tmp[25];
	int             ch;
	int             linect = 0, linect2 = 0;


	if(conn->single_log)
		snprintf(user_log, sizeof(user_log), "%s/log", logpath);
	else
		snprintf(user_log, sizeof(user_log), "%s/%s.log", logpath, sn);

	putchar('\n');

	logfile = fopen(user_log, "r");
	if (logfile == NULL)
		return;

	while (!feof(logfile)) {
		ch = fgetc(logfile);
		if (ch == '\n')
			linect++;
	}

	rewind(logfile);

	if (linect > lines) {
		while (!feof(logfile)) {
			ch = fgetc(logfile);
			if (ch == '\n') {
				linect2++;
				if (linect2 == (linect - lines))
					break;
			}
		}
	}
	while (!feof(logfile)) {
		if (fgets(buf, sizeof(buf), logfile) == NULL)
			break;

		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0;

		if (strlen(buf) > 24 && strchr(buf, ':') != NULL) {
			memset(tmp, 0, sizeof(tmp));
			strncpy(tmp, buf, 24);
			printf("%s", tmp);

			wordwrap_print(buf + 24, 24);
		} else {
			printf("                        ");
			wordwrap_print(buf, 24);
		}
	}

	fclose(logfile);
}

/* PROTO */
void
log_buddies_offline(void)
{
	struct BuddyList *tr;

	for (tr = buddylist; tr; tr = tr->next)
		log_event(EVENT_SIGNOFF, tr->sn, NULL);
}
