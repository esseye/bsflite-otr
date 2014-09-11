/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

extern struct Conn *conn;

/*
 * an alternate config parser works like getopt()
 *
 * obviously not thread-safe or anything like that. but then again, we don't
 * need those for bsf*.
 */

enum {
	CONFIG_USERNAME,
	CONFIG_PASSWORD,
	CONFIG_PROXYPORT,
	CONFIG_PROXYTYPE,
	CONFIG_PROXY,
	CONFIG_SQUELCHCONNECT,
	CONFIG_SQUELCHIDLE,
	CONFIG_SQUELCHAWAY,
	CONFIG_SINGLELOG,
	CONFIG_RESPONDIDLE,
	CONFIG_REPORTIDLE,
	CONFIG_FILTER,
	CONFIG_TIMESTAMPS,
	CONFIG_PORT,
	CONFIG_TITLE,
	CONFIG_BELL,
	CONFIG_IDLETIME,
	CONFIG_AUTORECONNECT,
	CONFIG_COLORS,
        CONFIG_OTR,
        CONFIG_DIR
};

#define bl_conf_int(x) atoi(x)
#define bl_conf_str(x) (x)

char           *bl_optarg;
FILE           *conf_file = NULL;
int             linecount;

/* PROTO */
int
read_conf(void)
{
	int             c;
	int             haspw = 0, hasuser = 0;
	char            buf[1024];

	c_options       ic[] = {
		{"username", CONFIG_USERNAME},
		{"password", CONFIG_PASSWORD},
		{"proxyport", CONFIG_PROXYPORT},
		{"proxytype", CONFIG_PROXYTYPE},
		{"proxy", CONFIG_PROXY},
		{"single_log", CONFIG_SINGLELOG},
		{"squelchconnect", CONFIG_SQUELCHCONNECT},
		{"squelchidle", CONFIG_SQUELCHIDLE},
		{"squelchaway", CONFIG_SQUELCHAWAY},
		{"respond_idle_only", CONFIG_RESPONDIDLE},
		{"report_idle", CONFIG_REPORTIDLE},
		{"netspeak_filter", CONFIG_FILTER},
		{"timestamps", CONFIG_TIMESTAMPS},
		{"oscarport", CONFIG_PORT},
		{"set_window_title", CONFIG_TITLE},
		{"bell_on_incoming", CONFIG_BELL},
		{"idletime", CONFIG_IDLETIME},
		{"colors", CONFIG_COLORS},
		{"auto_reconnect", CONFIG_AUTORECONNECT},
		{"otr", CONFIG_OTR},
		{NULL, -1}
	};

#if !defined(__MINGW32__) && !defined(__DJGPP__)
	char           *home;

#ifdef PLAN9
	if ((home = getenv("home")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/lib/bsflite/", home);
#elif defined(__BEOS__)
	snprintf(buf, sizeof(buf), "/boot/home/config/settings/bsflite/");
#else
	if ((home = getenv("HOME")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/.bsflite/", home);
#endif				/* PLAN9 */

	conn->configdir = strdup(buf);
        snprintf(buf, sizeof(buf), "%s%s", conn->configdir, "config");

#else
        conn->configdir = "";
	snprintf(buf, sizeof(buf), "config.txt");
#endif

	while (1) {
		c = bl_get_confopt(buf, ic);

		if (c == -1)
			break;

		/*
	         * -2 if an argument is missing
	         */
		if (c == -2)
			continue;

		/*
	         * -3 if the config file is missing
	         */
		if (c == -3) {
			return -1;
		}
		switch (c) {
		case CONFIG_USERNAME:
			conn->username = strdup(bl_conf_str(bl_optarg));
			hasuser = 1;
			break;
		case CONFIG_PASSWORD:
			conn->password = strdup(bl_conf_str(bl_optarg));
			haspw = 1;
			break;
		case CONFIG_PROXYPORT:
			conn->proxyport = bl_conf_int(bl_optarg);
			break;
		case CONFIG_PROXYTYPE:
			if (strncmp(bl_conf_str(bl_optarg), "socks5", 6) == 0) {
				conn->proxytype = PROXY_TYPE_SOCKS5;
			} else if (strncmp(bl_conf_str(bl_optarg), "https", 5) == 0) {
				conn->proxytype = PROXY_TYPE_HTTPS;
			} else {
				printf("** Unknown proxy type \"%s\" specified.\n",
				       bl_conf_str(bl_optarg));
			}
			break;
		case CONFIG_PROXY:
			conn->proxy = strdup(bl_conf_str(bl_optarg));
			break;
		case CONFIG_SINGLELOG:
			conn->single_log = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_SQUELCHCONNECT:
			conn->squelchconnect = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_SQUELCHIDLE:
			conn->squelchidle = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_SQUELCHAWAY:
			conn->squelchaway = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_RESPONDIDLE:
			conn->respond_idle_only = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_REPORTIDLE:
			conn->report_idle = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_FILTER:
			conn->netspeak_filter = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_TIMESTAMPS:
			conn->timestamps = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_PORT:
			conn->oscarport = bl_conf_int(bl_optarg);
			break;
		case CONFIG_TITLE:
			conn->set_window_title = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_BELL:
			conn->bell_on_incoming = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_IDLETIME:
			conn->idle_rep_time = bl_conf_int(bl_optarg);
			break;
		case CONFIG_COLORS:
			conn->colors = bl_conf_bool(bl_optarg);
			break;
		case CONFIG_AUTORECONNECT:
			conn->auto_reconnect = bl_conf_int(bl_optarg);
			break;
		case CONFIG_OTR:
			conn->otr = bl_conf_bool(bl_optarg);
			break;
		}
	}

	if (haspw && hasuser)
		return 1;
	else if (hasuser)
		return 2;
	else
		return 0;
}

/*
 * Plan 9's compiler doesn't like us to use typedef'd definitions in the
 * declaration, it seems.
 */

/* PROTO */
int
bl_get_confopt(char *filename, struct ConfigOptions * opts)
{
	static char     buf[1024];
	char           *tptr;	/* , *directive; */
	int             idx, token;

	if (conf_file == NULL) {
		conf_file = fopen(filename, "r");
		if (conf_file == NULL) {
			return -3;
		}
		linecount = 0;
	}
	while (1) {
		memset(buf, 0, sizeof(buf));
		linecount++;

		if (fgets(buf, sizeof(buf), conf_file) == NULL) {
			fclose(conf_file);
			conf_file = NULL;
			return -1;
		}
		if (buf[0] == '#')
			continue;

		tptr = strchr(buf, '\r');
		if (tptr != NULL) {
			tptr[0] = 0;
		} else {
			tptr = strchr(buf, '\n');
			if (tptr != NULL) {
				tptr[0] = 0;
			}
		}

		tptr = strchr(buf, ' ');
		if (tptr == NULL)
			return -2;

		for (idx = 0; opts[idx].directive != NULL; idx++) {
			if (strncmp
			    (buf, opts[idx].directive,
			     strlen(opts[idx].directive)) == 0) {
				token = opts[idx].token;
				bl_optarg = tptr + 1;
				return token;
			}
		}

		fprintf(stderr, "bl_get_confopt(): line %d, unknown option '%s'\n",
			linecount, buf);
		return -2;
	}

}

/* PROTO */
int
bl_conf_bool(char *arg)
{
	if (strncasecmp(arg, "true", 4) == 0)
		return 1;
	else
		return 0;
}
