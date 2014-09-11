/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

extern struct Conn *conn;

/* PROTO */
void
read_away_messages(void)
{
	FILE           *away;
	char            buf[1024], *tptr;
	struct AwayMessages *nn, *tv, *todel;

#if !defined(__MINGW32__) && !defined(__DJGPP__)
	char           *home;

#ifdef PLAN9
	if ((home = getenv("home")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/lib/bsflite/awaymessages", home);
#else
	if ((home = getenv("HOME")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/.bsflite/awaymessages", home);
#endif
#else
	snprintf(buf, sizeof(buf), "awaymsgs.txt");
#endif

	away = fopen(buf, "r");
	if (away == NULL)
		return;

	if (conn->awaymsgs != NULL) {
		for (tv = conn->awaymsgs; tv != NULL;) {
			todel = tv;
			tv = tv->next;
			free(todel);
		}

		conn->awaymsgs = NULL;
	}
	while (!feof(away)) {
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), away);

		tptr = strchr(buf, '\n');
		if (tptr != NULL)
			tptr[0] = 0;

		if (strlen(buf) == 0)
			break;

		nn = malloc(sizeof(struct AwayMessages));
		nn->message = strdup(buf);
		nn->next = NULL;

		if (conn->awaymsgs == NULL) {
			conn->awaymsgs = nn;
		} else {
			for (tv = conn->awaymsgs; tv->next != NULL; tv = tv->next);
			tv->next = nn;
		}
	}

	fclose(away);
}
