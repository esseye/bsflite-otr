/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"
#include <stdio.h>

#if defined(__MINGW32__) || defined(PLAN9)
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifdef __MINGW32__
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

#ifdef __DJGPP__
#include <conio.h>
#endif

extern char     inputbuf[512];
extern int      screen_cols;
extern int      screen_lines;
extern int      prompt_len;
extern struct BuddyList *buddylist;
extern struct Conn *conn;

#ifdef __MINGW32__
HANDLE          hOut;
#endif

/* PROTO */
void
addts(void)
{
	char            ts[11];
	struct tm      *now;
	time_t          t;

	set_color(COLOR_TIMESTAMP);

	t = time(0);
	now = localtime(&t);
	strftime(ts, 11, "(%H:%M:%S)", now);
	printf("%s", ts);

	set_color(0);
	return;
}

#if !defined(PLAN9)
#define TERMINAL_HIJINX
#endif

#ifdef TERMINAL_HIJINX

#ifdef __MINGW32__
#define TERMINAL_WINDOWS
#else

#ifdef __DJGPP__
#define TERMINAL_CONIO
#else
#define TERMINAL_VT100
#endif

#endif

#endif

/* PROTO */
void
eraseline(void)
{
	int             linelen = strlen(inputbuf) + prompt_len;
#ifdef TERMINAL_HIJINX
	int             numcols, numrows, x;
#ifdef TERMINAL_CONIO
	int             desiredx, desiredy;
#endif

#ifdef TERMINAL_WINDOWS
	COORD           nPos;
	CONSOLE_SCREEN_BUFFER_INFO wInfo;
	get_screen_size();
#endif

	numrows = linelen / screen_cols;
	numcols = linelen % screen_cols;

#ifdef TERMINAL_VT100
	if (numrows > 0)
		printf("\033[%dA", numrows);

	if (numcols > 0)
		printf("\033[%dD", numcols);
#endif
#ifdef TERMINAL_WINDOWS
	GetConsoleScreenBufferInfo(hOut, &wInfo);

	nPos.X = wInfo.dwCursorPosition.X - numcols;
	nPos.Y = wInfo.dwCursorPosition.Y - numrows;

	SetConsoleCursorPosition(hOut, nPos);
#endif

#ifdef TERMINAL_CONIO
	desiredx = wherex() - numcols;
	desiredy = wherey() - numrows;

	gotoxy(desiredx, desiredy);
#endif

	for (x = 0; x < linelen; x++)
		putchar(' ');

	/*
         * I suppose this could be enabled for all systems, but
         * Watcom-compiled Windows seems to be the only one actually needing
         * it.
         */

#ifdef WATCOM_WIN32
	fflush(stdout);
#endif

#ifdef TERMINAL_VT100
	if (numrows > 0)
		printf("\033[%dA", numrows);

	if (numcols > 0)
		printf("\033[%dD", numcols);
#endif
#ifdef TERMINAL_WINDOWS
	GetConsoleScreenBufferInfo(hOut, &wInfo);

	nPos.X = wInfo.dwCursorPosition.X - numcols;
	nPos.Y = wInfo.dwCursorPosition.Y - numrows;

	SetConsoleCursorPosition(hOut, nPos);
#endif
#ifdef TERMINAL_CONIO
	fflush(stdout);

	gotoxy(desiredx, desiredy);
#endif

#else
	int             x;

#ifdef PLAN9
	get_screen_size();
#endif

	for (x = 0; x < linelen; x++)
		printf("\b \b");
#endif

	set_color(0);
}

/* PROTO */
void
get_screen_size(void)
{
#if defined(__MINGW32__) || defined(__DJGPP__) || defined(PLAN9)
#ifdef __MINGW32__
	CONSOLE_SCREEN_BUFFER_INFO p;

	GetConsoleScreenBufferInfo(hOut, &p);

	screen_cols = (int) p.dwSize.X;
	screen_lines = (int) p.dwSize.Y;
#endif

#ifdef PLAN9
	screen_cols = getwidth();
	screen_lines = 25;
#endif

#ifdef __DJGPP__
	struct text_info ti;

	gettextinfo(&ti);

	screen_cols = ti.screenwidth;
	screen_lines = ti.screenheight;
#endif
#else
	struct winsize  scrsize;

	ioctl(fileno(stdin), TIOCGWINSZ, &scrsize);
	screen_cols = scrsize.ws_col;
	screen_lines = scrsize.ws_row;
#endif
}

/* PROTO */
void
wordwrap_print(char *str, int offset)
{
	char           *linebuf = NULL;
	char           *curline;
	int             firstline = 1;
	int             xx = 0, yy, jj;

#ifdef __MINGW32__
	get_screen_size();
#endif

	curline = str;

	for (;;) {
		if (curline[xx] == 0)
			break;
		if (curline[xx] != ' ' && (curline[xx] != '\n')) {
			xx++;
			continue;
		}
		yy = xx + 1;
		while ((curline[yy] != ' ') && (curline[yy] != '\n')
		       && (curline[yy] != 0))
			yy++;

		if ((curline[xx] == '\n') || (yy > (screen_cols - offset - 4))) {
			linebuf = malloc(xx + 1);

			strncpy(linebuf, curline, xx);
			linebuf[xx] = 0;

			if (!firstline) {
				for (jj = 0; jj < offset; jj++)
					putchar(' ');
			} else {
				firstline = 0;
			}

			printf("%s\n", linebuf);
			curline += xx + 1;
			xx = 0;

			free(linebuf);

			continue;
		}
		xx++;
	}

	if (!firstline)
		for (jj = 0; jj < offset; jj++)
			putchar(' ');

	printf("%s\n", curline);
}

/* PROTO */
void
wordwrap_print_echostr(char *str, char *echostr)
{
	char           *linebuf = NULL;
	char           *curline;
	int             offset;
	int             xx = 0, yy;

#ifdef __MINGW32__
	get_screen_size();
#endif

	offset = strlen(echostr) + 1;
	curline = str;

	for (;;) {
		if (curline[xx] == 0)
			break;
		if (curline[xx] != ' ') {
			xx++;
			continue;
		}
		yy = xx + 1;
		while (curline[yy] != ' ')
			if (curline[yy] == 0)
				break;
			else
				yy++;

		if (yy > (screen_cols - offset - 3)) {
			linebuf = malloc(xx + 1);

			strncpy(linebuf, curline, xx);
			linebuf[xx] = 0;

			set_color(COLOR_PROFILE_ECHOSTR);
			printf("%s", echostr);
			set_color(COLOR_PROFILE);
			printf(" %s\n", linebuf);
			curline += xx + 1;
			xx = 0;

			free(linebuf);
			continue;
		}
		xx++;
	}

	set_color(COLOR_PROFILE_ECHOSTR);
	printf("%s", echostr);
	set_color(COLOR_PROFILE);
	printf(" %s\n", curline);
	set_color(0);
}

#ifdef COLOR
/* PROTO */
void
set_color(int color)
{
#ifdef __MINGW32__
	int             bd, fg, bg;
	WORD            attr = 0;

	if (!conn->colors)
		return;

	if (color == 0) {
		SetConsoleTextAttribute(hOut,
					FOREGROUND_RED | FOREGROUND_GREEN |
					FOREGROUND_BLUE);
		return;
	}
	bd = color / 100;
	fg = color % 100;
	bg = (color % 100) / 10;

	if (bd)
		attr |= FOREGROUND_INTENSITY;

	switch (fg) {
	case 0:
		break;
	case 1:
		attr |= FOREGROUND_RED;
		break;
	case 2:
		attr |= FOREGROUND_GREEN;
		break;
	case 3:
		attr |= FOREGROUND_RED | FOREGROUND_GREEN;
		break;
	case 4:
		attr |= FOREGROUND_BLUE;
		break;
	case 5:
		attr |= FOREGROUND_BLUE | FOREGROUND_RED;
		break;
	case 6:
		attr |= FOREGROUND_BLUE | FOREGROUND_GREEN;
		break;
	case 7:
		attr |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
	}

	switch (bg) {
	case 0:
		break;
	case 1:
		attr |= BACKGROUND_RED;
		break;
	case 2:
		attr |= BACKGROUND_GREEN;
		break;
	case 3:
		attr |= BACKGROUND_RED | BACKGROUND_GREEN;
		break;
	case 4:
		attr |= BACKGROUND_BLUE;
		break;
	case 5:
		attr |= BACKGROUND_BLUE | BACKGROUND_RED;
		break;
	case 6:
		attr |= BACKGROUND_BLUE | BACKGROUND_GREEN;
		break;
	case 7:
		attr |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
		break;
	}

	SetConsoleTextAttribute(hOut, attr);
#else
	int             bd, fg;
#ifdef COLOR_DARKBG
	int             bg;
#endif

	if (!conn->colors)
		return;

	if (color == 0) {
		printf("\033[0m");
		return;
	}
	bd = color / 100;
	fg = color % 100;
#ifdef COLOR_DARKBG
	bg = (color % 100) / 10;

	printf("\033[%d;3%d;4%dm", bd, fg, bg);
#else
	printf("\033[%d;3%dm", bd, fg);
#endif
#endif
}
#endif

/* PROTO */
void
display_buddylist(void)
{
	struct BuddyList *trav;
	long            days, hours, minutes;

	for (trav = buddylist; trav != NULL; trav = trav->next) {
		printf("%s ", BUDDYLIST_ECHOSTR);

		if (trav->idle && trav->away)
			printf("%s", BUDDYLIST_GONE);
		else if (trav->idle)
			printf("%s", BUDDYLIST_IDLE);
		else if (trav->away)
			printf("%s", BUDDYLIST_AWAY);
		else
			printf("%s", BUDDYLIST_ONLINE);

		printf(" %-16s", trav->formattedsn);

		if (trav->idle) {
			days = trav->idletime / 1440;
			hours = (trav->idletime % 1440) / 60;
			minutes = trav->idletime % 60;
			printf("  (");
			if (days > 0)
				printf("%ld days, ", days);
			if (hours > 0)
				printf("%02ldh", hours);

			printf("%02ldm)", minutes);
		}
		printf("\n");
	}
}

#define p_echostr() {if(col == 1) printf("%s", BUDDYLIST_ECHOSTR);}

/* PROTO */
void
display_buddylist_sorted(void)
{
	struct BuddyList *trav;
	char            sncopy[17];
	int             run, col;
	int             numcols;


#ifdef __MINGW32__
	get_screen_size();
#endif

	numcols = (screen_cols / 24) - 1;

	if (numcols <= 0)
		numcols = 1;

	for (run = 0, col = 1; run < 4; run++) {
		for (trav = buddylist; trav != NULL; trav = trav->next) {
			set_color(COLOR_BUDDYLIST_ECHOSTR);

			if (run == 0) {
				if (!trav->idle && !trav->away) {
					p_echostr();
					printf(" %s", BUDDYLIST_ONLINE);
				} else {
					continue;
				}
			} else if (run == 1) {
				if (trav->idle && !trav->away) {
					p_echostr();
					printf(" %s", BUDDYLIST_IDLE);
				} else {
					continue;
				}
			} else if (run == 2) {
				if (!trav->idle && trav->away) {
					p_echostr();
					printf(" %s", BUDDYLIST_AWAY);
				} else {
					continue;
				}
			} else {
				if (trav->idle && trav->away) {
					p_echostr();
					printf(" %s", BUDDYLIST_GONE);
				} else {
					continue;
				}
			}

			set_color(0);
			memset(sncopy, 0, sizeof(sncopy));

			if (strlen(trav->formattedsn) > 16) {
				memcpy(sncopy, trav->formattedsn, 13);
#ifdef __OpenBSD__
				strlcat(sncopy, "...", sizeof(sncopy));
#else
				strcat(sncopy, "...");
#endif
			} else {
				memcpy(sncopy, trav->formattedsn,
				       strlen(trav->formattedsn));
			}

			switch (run) {
			case 0:
				set_color(COLOR_BUDDYLIST_ONLINE);
				break;
			case 1:
				set_color(COLOR_BUDDYLIST_IDLE);
				break;
			case 2:
				set_color(COLOR_BUDDYLIST_AWAY);
				break;
			case 3:
				set_color(COLOR_BUDDYLIST_GONE);
				break;
			}

			if (col < numcols) {
				printf(" %-16s  ", sncopy);
				col++;
			} else {
				printf(" %s", sncopy);
				set_color(0);
				printf("\n");
				col = 1;
			}

			set_color(0);
		}

		if (col != 1)
			printf("\n");

		col = 1;
	}

	set_color(0);
}

/* PROTO */
void
printspaces(int num)
{
	int             jj;

	for (jj = 0; jj < num; jj++)
		putchar(' ');
}

/* PROTO */
void
prettyprint(char *ptext, int offset)
{
	int             pos, x;
	char           *ttmp;

	pos = 0;
	while (pos < strlen(ptext)) {
		for (x = pos; x < strlen(ptext); x++) {
			if (x == strlen(ptext) - 1) {
				ttmp = malloc(x - pos + 2);
				strncpy(ttmp, ptext + pos, x - pos + 1);
				if (ttmp[x - pos] == '\n')
					ttmp[x - pos] = 0;

				ttmp[x - pos + 1] = 0;
				if (pos != 0)
					printspaces(offset);

				wordwrap_print(ttmp, offset);
				free(ttmp);
			} else if (ptext[x] == '\n') {
				ttmp = malloc(x - pos + 1);
				strncpy(ttmp, ptext + pos, x - pos);
				ttmp[x - pos] = 0;

				if (pos != 0)
					printspaces(offset);
				wordwrap_print(ttmp, offset);
				free(ttmp);
				break;
			}
		}
		pos = x + 1;
	}
}


/* PROTO */
void
prettyprint_echostr(char *ptext, char *echostr)
{
	int             pos, x;
	char           *ttmp;

	pos = 0;
	while (pos < strlen(ptext)) {
		for (x = pos; x < strlen(ptext); x++) {
			if (x == strlen(ptext) - 1) {
				ttmp = malloc(x - pos + 2);
				strncpy(ttmp, ptext + pos, x - pos + 1);
				if (ttmp[x - pos] == '\n')
					ttmp[x - pos] = 0;
				ttmp[x - pos + 1] = 0;
				wordwrap_print_echostr(ttmp, echostr);
				free(ttmp);
			} else if (ptext[x] == '\n') {
				ttmp = malloc(x - pos + 1);
				strncpy(ttmp, ptext + pos, x - pos);
				ttmp[x - pos] = 0;
				wordwrap_print_echostr(ttmp, echostr);
				free(ttmp);
				break;
			}
		}
		pos = x + 1;
	}
}

/* PROTO */
void
set_title(char *title)
{
	char           *termtype = getenv("TERM");

	if (conn->windowtitle != NULL)
		free(conn->windowtitle);

	if (strncmp(termtype, "xterm", 5) == 0) {
		printf("\033]0;%s\007", title);
	} else if (strncmp(termtype, "screen", 6) == 0) {
		printf("\033_%s\033\\", title);
	}
	conn->windowtitle = strdup(title);

	fflush(stdout);
}
