/**
 ** bsflite - bs-free AIM client
 **           ultralight version.
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

#ifndef __MINGW32__
#include <termios.h>
#else
#include <conio.h>
#endif

#ifdef PLAN9
#include <fcntl.h>
#endif

#ifdef __DJGPP__
void            clrscr(void);
int             getch(void);
int             kbhit(void);
#endif

#include <signal.h>

struct Conn    *conn;
struct BuddyList *buddylist;
struct Waiting *waiting = NULL;
char            buf[256];
char            inputbuf[512];
int             proto = 0;

#ifndef __MINGW32__
struct termios  t_attr;
struct termios  saved_attr;
int             istat, attrs_saved;
#endif
int             screen_cols;
int             screen_lines;
int             prompt_len;
int             ratestatus = 0;
time_t          last_status_time = 0;
time_t          last_keystroke_time;

/* PROTO */
void
addtoinputbuf(char inchr)
{
	conn->istyping = 1;

	if (strlen(inputbuf) == 511)
		return;

	putchar(inchr);
	inputbuf[strlen(inputbuf)] = inchr;
	fflush(stdout);
}

int
main(void)
{
	int             xx;
	int             needpass = 0, needuser = 0;
	struct Waiting *wtemp, *wtemp2;
	conn = malloc(sizeof(struct Conn));
	conn->clist = NULL;
	conn->squelchidle = 0;
	conn->isidle = 0;
	conn->rawinput = 0;
	conn->single_log = 0;
	conn->squelchaway = 0;
	conn->squelchconnect = 0;
	conn->respond_idle_only = 0;
	conn->report_idle = 1;
	conn->netspeak_filter = 0;
	conn->awaymsgs = NULL;
	conn->awaymsg = NULL;
	conn->timestamps = 1;
	conn->colors = 0;
	conn->auto_reconnect = 0;
        conn->otr = 0;
	conn->bell_on_incoming = 0;
	conn->idle_rep_time = 600;
	conn->windowtitle = NULL;
	conn->username = NULL;
	conn->password = NULL;
#ifdef __MINGW32__
	conn->set_window_title = 1;
#else
	conn->set_window_title = 0;
#endif
	conn->proxy = NULL;
	conn->proxytype = PROXY_TYPE_NONE;
	conn->oscarport = 0;

	switch (read_conf()) {
	case -1:
		/*
	         * config file missing
	         */
		b_echostr_s();
		printf("Could not open config file.\n");
	case 0:
		needuser = 1;
		needpass = 1;
		break;
	case 1:
		break;
	case 2:
		needpass = 1;
		break;
	}

	setup_tty();
	get_screen_size();
	/* signal(SIGINT, SIG_IGN); */
	signal(SIGINT, &sigint_handler);

#if !defined(__MINGW32__) && !defined(__DJGPP__)
#ifndef PLAN9
	signal(SIGWINCH, &sigwinch_handler);
#endif
	signal(SIGUSR1, &sigusr1_handler);
	signal(SIGTSTP, SIG_IGN);
#endif

	open_log_dir();

	conn->lastsn = 0;
	conn->istyping = 0;
	conn->isaway = 0;
	conn->isinvisible = 0;
	conn->buddiesonline = 0;
	buddylist = 0;

	b_echostr_s();
	printf("bsflite %s started.\n", BSFLITE_VERSION);
	memset(inputbuf, 0, sizeof(inputbuf));

	if (needuser) {
		b_getusername();
	} else {
		b_echostr();

		if (conn->timestamps) {
			putchar(' ');
			addts();
		}
		printf(" Logging in as %s...\n", conn->username);
	}

	if (needpass) {
		b_getpassword();
	}
	read_away_messages();

        if (conn->otr) {
        	init_otr();
        }

	create_new_connection();

	if (conn->proxytype != PROXY_TYPE_NONE) {
		imcomm_set_proxy(conn->clist->conn, conn->proxytype, conn->proxy,
				 (uint16_t) conn->proxyport);
	}
	if (conn->oscarport != 0) {
		imcomm_set_oscar_port(conn->clist->conn, conn->oscarport);
	}
	imcomm_im_signon(conn->clist->conn, conn->username, conn->password);
	conn->conn = conn->clist->conn;

	signal(SIGINT, SIG_IGN);

	last_keystroke_time = time(NULL);
	show_prompt();

	while (1) {
		fd_set          readfs;
		unsigned char   inchr;
		int             nchr;
		struct timeval  tm;

		FD_ZERO(&readfs);

#if defined(__MINGW32__) || defined(__DJGPP__)
		tm.tv_sec = 0;
		tm.tv_usec = 500;
#else
		tm.tv_sec = 2;
		tm.tv_usec = 500000;
		FD_SET(STDIN_FILENO, &readfs);
#endif

		if (imcomm_select(STDIN_FILENO + 1, &readfs, NULL, NULL, &tm) !=
		    IMCOMM_RET_OK) {
			if (errno == EINTR)
				continue;
		}
#if defined(__MINGW32__) || defined(__DJGPP__)
		if (kbhit()) {
			inchr = getch();
			nchr = 1; /* otherwise it'll never enter the
				   * input loop. -- fixed 0.84
				   */
#else
		if (FD_ISSET(STDIN_FILENO, &readfs)) {
			nchr = read(0, &inchr, 1);
#endif
			if (nchr == 1) {
				if (conn->report_idle) {
					last_keystroke_time = time(NULL);

					if (conn->isidle == 1) {
						if (conn->conn)
							imcomm_set_idle_time(conn->conn, 0);
						conn->isidle = 0;
						eraseline();
						show_prompt();
					}
				}
				switch (inchr) {
				case '\n':
				case '\r':
					parse_input();
					memset(inputbuf, 0, sizeof(inputbuf));
					conn->istyping = 0;
					conn->rawinput = 0;

					for (wtemp = waiting; wtemp != NULL;) {
						wtemp2 = wtemp;
						wtemp = wtemp->next;
						free(wtemp2->sn);
						free(wtemp2);
					}
					waiting = NULL;
					show_prompt();
					break;
				case 18:	/* CTRL-R */
					if (conn->rawinput == 1)
						conn->rawinput = 0;
					else
						conn->rawinput = 1;

					eraseline();
					show_prompt();
					break;
				case 21:	/* CTRL-U */
					eraseline();
					memset(inputbuf, 0, sizeof(inputbuf));

					conn->istyping = 0;
					conn->rawinput = 0;

					show_prompt();
					break;
				case 12:

#if !defined(__MINGW32__) && !defined(PLAN9)
#ifdef __DJGPP__
					clrscr();
#else
					printf("\033[2J\033[1;1H");
#endif
					show_prompt();
#endif
					break;
				case '\t':
					if (!
					    (inputbuf[0] == 'm' || inputbuf[0] == 'i'
					     || inputbuf[0] == 'l' || inputbuf[0] == 'A'
					     || inputbuf[0] == 'P' || inputbuf[0] == 'p'
					     || inputbuf[0] == 's' || inputbuf[0] == 'O'
					     || inputbuf[0] == 'S' || inputbuf[0] == 'M'
					     || inputbuf[0] == 'o'))
						break;
					else {
						int             spaces = 0;
						int             match = 0;
						struct BuddyList *trav;

						for (xx = 0; xx < strlen(inputbuf); xx++)
							if (inputbuf[xx] == ' ') {
								spaces = 1;
								break;
							}
						if (spaces)
							break;
						trav = buddylist;
						while (trav != NULL) {
							if (strncmp
							    (trav->sn, inputbuf + 1,
							     strlen(inputbuf) - 1) == 0) {
								match++;
							}
							trav = trav->next;
						}
						if (match > 1)
							break;
						else if (match == 0)
							break;
						else {
							trav = buddylist;
							while (trav != NULL) {
								if (strncmp
								    (trav->sn, inputbuf + 1,
								     strlen(inputbuf) - 1) == 0)
									break;
								trav = trav->next;
							}

							eraseline();

							memcpy(inputbuf + 1, trav->sn, strlen(trav->sn));
							if (inputbuf[0] == 'm')
								inputbuf[strlen(inputbuf)] = ' ';
							show_prompt();
						}
					}
					break;
				case '\b':
				case 127:
				case 4:
					if (inputbuf[0] == 0) {
						conn->istyping = 0;
						break;
					}
					printf("\b \b");

					inputbuf[strlen(inputbuf) - 1] = 0;
					fflush(stdout);
					break;
				case 2:
					if (strlen(inputbuf) > 509)
						break;
					printf("<br>");
					fflush(stdout);
#if defined(__OpenBSD__)
					strlcat(inputbuf, "<br>", sizeof(inputbuf));
#else
					strcat(inputbuf, "<br>");
#endif

					break;
				case '"':
					if (!conn->rawinput) {
						if (strlen(inputbuf) > 507)
							break;
						printf("&quot;");
						fflush(stdout);

#if defined(__OpenBSD__)
						strlcat(inputbuf, "&quot;", sizeof(inputbuf));
#else
						strcat(inputbuf, "&quot;");
#endif
					} else {
						addtoinputbuf(inchr);
					}

					break;
				case '<':
					if (!conn->rawinput) {
						if (strlen(inputbuf) > 509)
							break;
						printf("&lt;");
						fflush(stdout);

#if defined(__OpenBSD__)
						strlcat(inputbuf, "&lt;", sizeof(inputbuf));
#else
						strcat(inputbuf, "&lt;");
#endif
					} else {
						addtoinputbuf(inchr);
					}
					break;
				case '>':
					if (!conn->rawinput) {
						if (strlen(inputbuf) > 509)
							break;
						printf("&gt;");
						fflush(stdout);

#if defined(__OpenBSD__)
						strlcat(inputbuf, "&gt;", sizeof(inputbuf));
#else
						strcat(inputbuf, "&gt;");
#endif

					} else {
						addtoinputbuf(inchr);
					}
					break;
				case '&':
					if (!conn->rawinput) {
						if (strlen(inputbuf) > 508)
							break;
						printf("&amp;");
						fflush(stdout);

#if defined(__OpenBSD__)
						strlcat(inputbuf, "&amp;", sizeof(inputbuf));
#else
						strcat(inputbuf, "&amp;");
#endif
					} else {
						addtoinputbuf(inchr);
					}
					break;
				case 'r':
				case 'I':
				case 'L':
					if (inputbuf[0] == 0) {
						if (conn->lastsn) {
							conn->istyping = 1;
							if (inchr == 'r')
								snprintf(inputbuf, sizeof(inputbuf), "m%s ",
									 conn->lastsn);
							else if (inchr == 'I')
								snprintf(inputbuf, sizeof(inputbuf), "i%s",
									 conn->lastsn);
							else
								snprintf(inputbuf, sizeof(inputbuf), "l%s",
									 conn->lastsn);

							printf("%s", inputbuf);

							fflush(stdout);
						} else if (conn->single_log && inchr == 'L') {
							snprintf(inputbuf, sizeof(inputbuf), "lg");
							printf("%s", inputbuf);
							fflush(stdout);
						}
						break;
					}
				default:
					if (inchr < 32)
						break;

					addtoinputbuf(inchr);
					break;
				}
			}
		}
		if (ratestatus == 1) {
			if ((time(NULL) - last_status_time) > 30) {
				eraseline();
				ratestatus = 0;
				show_prompt();
			}
		}
		if (conn->report_idle && nchr == 1) {
			if (conn->isidle == 0) {
				if (time(NULL) - last_keystroke_time > conn->idle_rep_time) {
					conn->isidle = 1;
					if (conn->conn)
						imcomm_set_idle_time(conn->conn,
								time(NULL) -
						       last_keystroke_time);
					eraseline();
					show_prompt();
				}
			}
		}
	}
}

/*
 * this is from micq - mreadline.c
 *
 * rewrite this sometime.
 */
/* PROTO */
void
setup_tty(void)
{
#ifdef __MINGW32__
	extern HANDLE   hOut;
#endif

#if !defined(__MINGW32__) && !defined(PLAN9)
	istat = 0;
	if (tcgetattr(fileno(stdin), &t_attr) != 0)
		return;
	if (!attrs_saved) {
		saved_attr = t_attr;
		attrs_saved = 1;
	}
	t_attr.c_lflag &= ~(ECHO | ICANON);
#ifdef __BEOS__
	t_attr.c_cc[VMIN] = 0;
#else
	t_attr.c_cc[VMIN] = 1;
#endif
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &t_attr) != 0)
		perror("can't change tty modes.");

	if (conn->set_window_title) {
		set_title("bsflite");
	}
#else
#ifdef PLAN9
	int             consctl, label;

	label = open("/dev/label", O_WRONLY);
	if (label >= 0) {
		write(label, "bsflite", 7);
	}
	consctl = open("/dev/consctl", O_WRONLY);
	if (consctl < 0) {
		perror("Can't open consctl");
		exit(-1);
	}
	write(consctl, "rawon", 5);
#endif
#ifdef __MINGW32__
	SetConsoleTitle("bsflite");
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
#endif
}

/* PROTO */
void
restore_tty(void)
{
#ifndef __MINGW32__
	tcsetattr(fileno(stdin), TCSAFLUSH, &saved_attr);
#endif
}

/* PROTO */
void
show_prompt(void)
{
	struct Waiting *wtemp;

	if (conn->isaway)
		prompt_len = strlen(BSF_PROMPT_AWAY) + 1;
	else if (conn->isidle)
		prompt_len = strlen(BSF_PROMPT_IDLE) + 1;
	else if (conn->rawinput)
		prompt_len = strlen(BSF_PROMPT_RAWINPUT) + 1;
	else
		prompt_len = strlen(BSF_PROMPT) + 1;

	if (ratestatus == 1) {
		printf("<!> ");
		prompt_len += 4;
	} else if (ratestatus == 2) {
		printf("<!!> ");
		prompt_len += 5;
	}
	set_color(COLOR_WAITING);

	if (waiting != NULL) {
		for (wtemp = waiting; wtemp != NULL; wtemp = wtemp->next) {
			prompt_len += strlen(wtemp->sn) + 3;
#ifdef MESSAGE_QUEUE
			if (waiting->mqueue != NULL) {
				printf("[%s*] ", wtemp->sn);
				prompt_len += 1;
			} else {
				printf("[%s] ", wtemp->sn);
			}
#else
			printf("[%s] ", wtemp->sn);
#endif
		}
	}
	set_color(COLOR_PROMPT);
	if (conn->isaway)
		printf("%s", BSF_PROMPT_AWAY);
	else if (conn->isidle)
		printf("%s", BSF_PROMPT_IDLE);
	else if (conn->rawinput)
		printf("%s", BSF_PROMPT_RAWINPUT);
	else
		printf("%s", BSF_PROMPT);
	set_color(0);
	printf(" %s", inputbuf);
	fflush(stdout);
}

/* PROTO */
void
sigwinch_handler(int a)
{
	get_screen_size();
}

/* PROTO */
void
sigint_handler(int a)
{
	restore_tty();
	exit(1);
}

/* PROTO */
void
sigusr1_handler(int a)
{
	set_profile(conn->clist->conn);
	read_away_messages();
}

/* PROTO */
void
set_profile(void *handle)
{
	FILE           *prof;
	char            buf[1024];
#if !defined(__MINGW32__) && !defined(__DJGPP__)
	char           *home;
#endif
	char           *profile;
	int             ch;
	int             len = 0;
	size_t          proflen;

#if !defined(__MINGW32__) && !defined(__DJGPP__)

#ifdef PLAN9
	if ((home = getenv("home")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/lib/bsflite/profile", home);
#else
	if ((home = getenv("HOME")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/.bsflite/profile", home);
#endif				/* PLAN9 */

#else
	snprintf(buf, sizeof(buf), "profile.htm");
#endif
	prof = fopen(buf, "r");
	if (prof == NULL) {
		char           *profdef =
		"<a href=\"http://bsflite.sf.net/\">bsf</a>: ultralight AIM.";
		size_t          profstlen;
		char           *profst;

		profstlen = strlen(profdef) + strlen(ID_STR) + 1;
		profst = malloc(profstlen);

#if defined(__OpenBSD__)
		strlcpy(profst, profdef, profstlen);
		strlcat(profst, ID_STR, profstlen);
#else
		strcpy(profst, profdef);
		strcat(profst, ID_STR);
#endif

		imcomm_set_profile(handle, profst);
		free(profst);
		return;
	}
	while (1) {
		if (fgetc(prof) == EOF)
			if (feof(prof))
				break;
		len++;
	}

	rewind(prof);

	proflen = len + strlen(ID_STR) + 1;
	profile = malloc(proflen);
	memset(profile, 0, proflen);
	while (1) {
		if ((ch = fgetc(prof)) == EOF)
			if (feof(prof))
				break;
		profile[strlen(profile)] = ch;
	}

#ifdef __OpenBSD__
	strlcat(profile, ID_STR, proflen);
#else
	strcat(profile, ID_STR);
#endif

	imcomm_set_profile(handle, profile);
	fclose(prof);
	free(profile);
}

/* PROTO */
void
set_icon(void *handle)
{
	FILE           *icon;
	char            buf[1024];
#if !defined(__MINGW32__) && !defined(__DJGPP__)
	char           *home;
#endif
	uint8_t        *icondata;
	int             ch;
	int             len = 0, read = 0;

#if !defined(__MINGW32__) && !defined(__DJGPP__)

#ifdef PLAN9
	if ((home = getenv("home")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/lib/bsflite/icon", home);
#else
	if ((home = getenv("HOME")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/.bsflite/icon", home);
#endif				/* PLAN9 */

#else
	snprintf(buf, sizeof(buf), "icon.bin");
#endif
	icon = fopen(buf, "rb");
	if (icon == NULL) {
		return;
	}
	while (1) {
		if (fgetc(icon) == EOF)
			if (feof(icon))
				break;
		len++;
	}

	rewind(icon);
	icondata = malloc(len);

	while (1) {
		if ((ch = fgetc(icon)) == EOF)
			if (feof(icon))
				break;
		icondata[read] = (unsigned char) ch;
		read++;
	}

	/* icon support is pretty broken */

	/*
         * imcomm_upload_icon(handle, icondata, (uint16_t) len);
         */
	fclose(icon);
	free(icondata);
}

/* PROTO */
void
load_buddylist(void *handle)
{
	FILE           *buddy;
	char            buf[1024];
#if !defined(__MINGW32__) && !defined(__DJGPP__)
	char           *home;

#ifdef PLAN9
	if ((home = getenv("home")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/lib/bsflite/buddies", home);
#else
	if ((home = getenv("HOME")) == NULL)
		home = ".";

	snprintf(buf, sizeof(buf), "%s/.bsflite/buddies", home);
#endif				/* PLAN9 */

#else
	snprintf(buf, sizeof(buf), "buddies.txt");
#endif
	buddy = fopen(buf, "r");
	if (buddy == NULL)
		return;

	while (!feof(buddy)) {
		if (fgets(buf, sizeof(buf), buddy) == NULL)
			break;

		if (buf[strlen(buf) - 2] == '\r')
			buf[strlen(buf) - 2] = 0;

		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0;

		imcomm_im_add_buddy(handle, buf);
	}

	fclose(buddy);
}

/* PROTO */
void
error_callback(void *handle, int event, uint32_t data)
{
	if (event != IMCOMM_RATE_LIMIT_WARN) {
		eraseline();
		b_echostr_s();

		if (conn->timestamps) {
			addts();
			putchar(' ');
		}
	}
	switch (event) {
	case IMCOMM_STATUS_CONNECTED:
		printf("Connected.\n");
		load_buddylist(handle);
		set_icon(handle);
		break;
	case IMCOMM_ERROR_INVALID_LOGIN:
		printf("Login failed.\n");
		break;
	case IMCOMM_ERROR_DISCONNECTED:
		printf("Disconnected.\n");
		imcomm_delete_handle(handle);
		break;
	case IMCOMM_ERROR_OTHER_SIGNON:
		wordwrap_print
			("You've been disconnected because you signed on at a different location.",
			 14);
		break;
	case IMCOMM_STATUS_AUTHDONE:
		printf("Authentication succeeded.\n");
		break;
	case IMCOMM_STATUS_MIGRATIONDONE:
		printf("Migration completed.\n");
		break;
	case IMCOMM_ERROR_USER_OFFLINE:
		printf("Error: Recipient is not online.\n");
		break;
	case IMCOMM_RATE_LIMIT_WARN:
		break;
	case IMCOMM_ERROR_PROXY:
		switch (data) {
		case PROXY_ERROR_AUTH:
			printf("Proxy requires authentication.\n");
			break;
		case PROXY_ERROR_CONNECT:
			printf("Could not connect to proxy.\n");
			break;
		case PROXY_ERROR_PROXYCONNECT:
			printf("Proxy could not connect to destination.\n");
			break;
		case PROXY_ERROR_UNKNOWN:
			printf("Unknown proxy error or response.\n");
			break;
		}
		break;
	case IMCOMM_WARN_PAUSE:
		printf("Server sent PAUSE request.\n");
		break;
	case IMCOMM_WARN_UNPAUSE:
		printf("Server sent UNPAUSE request.\n");
		break;
	default:
		printf("ERROR: Unknown error type: %d\n", event);
		break;
	}

	if(event != IMCOMM_RATE_LIMIT_WARN)
		show_prompt();
}

/* PROTO */
void
handle_deleted(void *handle)
{
	struct ConnPtr *temp, *trav;

	delete_buddylist(buddylist);

	buddylist = NULL;
	conn->buddiesonline = 0;
	conn->conn = NULL;

	if (conn->clist->conn == handle) {
		temp = conn->clist;
		conn->clist = conn->clist->next;

		if (temp->username)
			free(temp->username);

		free(temp);
	} else {
		for (trav = conn->clist; trav->next;) {
			if (trav->next->conn == handle) {
				temp = trav->next;
				trav->next = trav->next->next;

				if (temp->username)
					free(temp->username);

				free(temp);
			}
		}
	}

	if (conn->auto_reconnect != 0) {

		eraseline();
		b_echostr_s();

		if (conn->timestamps) {
			addts();
			putchar(' ');
		}
		printf("Reconnecting in %d seconds...", conn->auto_reconnect);
		fflush(stdout);
		sleep(conn->auto_reconnect);
		printf("\n");

		create_new_connection();

		if (conn->proxytype != PROXY_TYPE_NONE) {
			imcomm_set_proxy(conn->clist->conn, conn->proxytype,
				   conn->proxy, (uint16_t) conn->proxyport);
		}
		if (conn->oscarport != 0) {
			imcomm_set_oscar_port(conn->clist->conn, conn->oscarport);
		}
		imcomm_im_signon(conn->clist->conn, conn->username,
				 conn->password);
		conn->conn = conn->clist->conn;

		if (conn->isaway)
			imcomm_set_away(conn->conn, conn->awaymsg);

	}
}

/* PROTO */
void
b_getusername(void)
{
	char            ch;
	char            passbuf[256];

	memset(passbuf, 0, sizeof(passbuf));
	printf("%s Enter screen name: ", BSF_PROMPT);
	fflush(stdout);
	while (strlen(passbuf) < sizeof(passbuf)) {
#if defined(__MINGW32__) || defined(__DJGPP__)
		ch = getch();
#else
		ch = getchar();
#endif
		if (ch == '\b' || ch == 127 || ch == 4) {
			if (passbuf[0] == 0)
				continue;
			printf("\b \b");
			passbuf[strlen(passbuf) - 1] = 0;
			fflush(stdout);
			continue;
		}
		if (ch == '\n' || ch == '\r')
			break;
		if (ch == ' ' || ch < 32)
			continue;
		if (ch >= 'A' && ch <= 'Z')
			ch = tolower(ch);

		passbuf[strlen(passbuf)] = ch;
		putchar(ch);
		fflush(stdout);
	}

	conn->username = strdup(passbuf);
	putchar('\n');
}

/* PROTO */
void
b_getpassword(void)
{

	char            ch;
	char            passbuf[256];
	int             nchr;

	memset(passbuf, 0, sizeof(passbuf));
	printf("%s Enter password: ", BSF_PROMPT);
	fflush(stdout);
	while (strlen(passbuf) < sizeof(passbuf)) {
#if defined(__MINGW32__) || defined(__DJGPP__)
		ch = getch();
#else
		/* ch = getchar(); */
		nchr = read(0, &ch, 1);
		if (nchr < 1)
			continue;
#endif
		if (ch == '\b' || ch == 127 || ch == 4) {
			if (passbuf[0] == 0)
				continue;
			printf("\b \b");
			passbuf[strlen(passbuf) - 1] = 0;
			fflush(stdout);
			continue;
		}
		if (ch == '\n' || ch == '\r')
			break;
		passbuf[strlen(passbuf)] = ch;
		putchar('.');
		fflush(stdout);
	}

	conn->password = strdup(passbuf);
	putchar('\n');
}
