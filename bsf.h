/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#ifdef PLAN9
#define _BSD_EXTENSION
#define _POSIX_EXTENSION
#define _POSIX_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef PLAN9
#include <strings.h>
#endif
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <libotr/instag.h>
#include <libotr/proto.h>
#include <libotr/userstate.h>
#include <libotr/message.h>
#include <libotr/privkey.h>
#include "imcomm/imcomm.h"


#ifdef __MINGW32__
#ifdef COLOR
#define COLOR_DARKBG
#endif
#endif

#define ID_STR ""

#define BSFLITE_VERSION "0.85"

struct ConnPtr {
	void           *conn;
	char           *username;
	struct ConnPtr *next;
};

struct AwayMessages {
	char           *message;
	struct AwayMessages *next;
};

struct Conn {
	struct ConnPtr *clist;

	/*
         * This is the current connection. All of them are listed within
         * clist.
         */
	void           *conn;
	char           *username;
	char           *server;
	char           *password;
	char           *lastsn;
	char           *awaymsg;
	char           *proxy;
	char           *windowtitle;
	char           *configdir;
	int             proxyport;
	int             proxytype;
	int             oscarport;
	int             istyping;
	int             isaway;
	int             isidle;
	int             isinvisible;
	int             rawinput;
	int             buddiesonline;
	int             single_log;
	int             squelchaway;
	int             squelchidle;
	int             squelchconnect;
	int             respond_idle_only;
	int             report_idle;
	int             netspeak_filter;
	int             set_window_title;
	int             timestamps;
	int             bell_on_incoming;
	int             idle_rep_time;
	int             colors;
	int             auto_reconnect;
	int             otr;
	OtrlInsTag	*otr_instag;
	struct AwayMessages *awaymsgs;
};

#undef MESSAGE_QUEUE		/* XXX */

#ifdef MESSAGE_QUEUE
struct MessageQueue {
	char           *message;
	char           *whom;
	time_t          when;
	struct MessageQueue *next;
};
#endif

struct Waiting {
	char           *sn;
#ifdef MESSAGE_QUEUE
	struct MessageQueue *mqueue;
#endif
	struct Waiting *next;
};

struct BuddyList {
	char           *sn;
	char           *formattedsn;
	int             idle;
	int             away;
	int		otr;
	ConnContext	*otr_context;
	long            idletime;
	struct BuddyList *next;
	struct BuddyList *prev;
};

typedef struct ConfigOptions {
	char           *directive;
	int             token;
}               c_options;

#define EVENT_IM 0
#define EVENT_SIGNON 1
#define EVENT_SIGNOFF 2
#define EVENT_IMSEND 3
#define EVENT_IM_AUTORESPONSE 4

#ifndef NO_COLOR_SUPPORT
#define COLOR
#define COLOR_DARKBG
#endif

#include "colors.h"

#ifndef COLOR
#define set_color(x) {;}
#define b_echostr() {printf("::");}
#define b_echostr_s() {printf(":: ");}
#else
#define b_echostr() {set_color(COLOR_ECHOSTR); printf("::"); set_color(0);}
#define b_echostr_s() {set_color(COLOR_ECHOSTR); printf(":: "); set_color(0);}
#endif

/************************************
 ** These are configurable formats **
 ************************************/

/**
 ** HTML formats for outgoing IM's
 **
 ** These'll wrap around your outgoing messages, so you
 ** can use these to set a font style. I like Georgia.
 **/
/* #define SEND_FORMAT_BEGIN "<font face=\"georgia\" size=\"-1\">" */
/* #define SEND_FORMAT_END "</font>" */

#define SEND_FORMAT_BEGIN ""
#define SEND_FORMAT_END ""

/**
 ** Prompt, very simple.
 **
 ** I don't think there are any other apps that use ::
 **/
#define BSF_PROMPT ">>"
#define BSF_PROMPT_AWAY "@>"
#define BSF_PROMPT_IDLE "%>"
#define BSF_PROMPT_RAWINPUT "R>"

/**
 ** >> is the old prompt (pre-0.71)
 **/
/* #define BSF_PROMPT ">>" */

/* #define BSF_PROMPT "bsf>" */

#define PROFILE_ECHOSTR "**"
#define BUDDYLIST_ECHOSTR "  "
#define BUDDYLIST_AWAY "[a]"
#define BUDDYLIST_IDLE "[i]"

/* "gone" is away and idle */
#define BUDDYLIST_GONE "[g]"

#define BUDDYLIST_ONLINE "[o]"

/**
 ** Path for HTML profile dumps
 ** This is needed if you define
 ** DUMP_PROFILE
 **
 ** Please change this; this applies to only
 ** my machine.
 **/
#define PROFILE_DUMP_PATH "profiledump.html"

/*
 * I never had an issue with mingw32 before, but for some reason the version
 * I installed doesn't know ssize_t.
 *
 * I guess this is useful for some other platforms as well.
 */
#if !defined(ssize_t) && !defined(PLAN9)
#define ssize_t int
#endif

#include "protos.h"
