/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

#if defined(__MINGW32__) || defined(__DJGPP__)
#include <conio.h>
#endif

extern char     inputbuf[512];
extern struct Conn *conn;
extern struct BuddyList *buddylist;

extern OtrlUserState userstate;
extern OtrlMessageAppOps ui_ops;
extern char *otr_proto;


typedef struct InputCallbacks {
	char           *command;
	int             length;
	void            (*callback) ();
}               i_callbacks;

/* PROTO */
void
parse_input(void)
{
	/*
         * Any commands not listed here are handled directly in the keyboard
         * input routine in bsf.c.
         */

	i_callbacks     ic[] = {
		/*
	         * "m" and "i" are the most used commands, I find
	         *
	         * let's put them sooner to avoid too many iterations on for()
	         * below.
	         */

		{"m", 1, input_send_message},
		{"i", 1, input_get_info},

		{"A", 1, input_get_away},
		{"P", 1, input_get_profile},
		{"G", 1, input_set_predefaway},
		{"a", 1, input_add_buddy},
		{"d", 1, input_delete_buddy},
		{"g", 1, input_set_away},
		{"h", 1, input_show_help},
		{"l", 1, input_show_log},
		{"p", 1, input_paste},
		{"q!", 2, input_quit},
		{"R!", 2, input_reconnect},
		{"v", 1, input_set_invisible},
		{"w", 1, input_show_buddies},
		{"z!", 2, input_reload_profile},
		{"?", 1, input_show_help},
		{"O", 1, input_show_otr},
		{"o", 1, input_stop_otr},
		{"S", 1, input_send_smp},
		{NULL, 0, NULL}
	};

	int             ix;

	for (ix = 0; ic[ix].command != NULL; ix++) {
		if (strncmp(inputbuf, ic[ix].command, ic[ix].length) == 0) {
			if (inputbuf[ic[ix].length] == ' ')
				ic[ix].callback(inputbuf + ic[ix].length + 1);
			else
				ic[ix].callback(inputbuf + ic[ix].length);

			return;
		}
	}

	printf("\n");
}

/* PROTO */
void
input_quit(char *arg)
{
	struct ConnPtr *trav, *temp;

	restore_tty();

	if (conn->otr) {
		printf("\n");
		otr_cleanup();
	}

	log_buddies_offline();

	imcomm_delete_handle_now(conn->conn);

	if (conn->clist->conn == conn->conn) {
		temp = conn->clist;
		conn->clist = conn->clist->next;

		if (temp->username)
			free(temp->username);

		free(temp);
	} else {
		for (trav = conn->clist; trav->next;) {
			if (trav->next->conn == conn->conn) {
				temp = trav->next;
				trav->next = trav->next->next;

				if (temp->username)
					free(temp->username);

				free(temp);
			}
		}
	}

	delete_buddylist(buddylist);

	if (conn->username)
		free(conn->username);

	if (conn->password)
		free(conn->password);

	if (conn->awaymsg)
		free(conn->awaymsg);

	if (conn->lastsn)
		free(conn->lastsn);

	free(conn);

	printf("\n");

	exit(0);
}

/* PROTO */
void
input_reload_profile(char *arg)
{
	if (conn->conn == NULL)
		return;

	printf("\n");
	b_echostr_s();

	printf("Reloading away messages and profile.\n");

	set_profile(conn->conn);
	read_away_messages();
}

/* PROTO */
void
input_send_message(char *arg)
{
	char           *msg, *msg_strip, *temp, *sn;
	char           *fullmsg;
	size_t          fullmsg_len;
	int             offset;

	if (conn->conn == NULL)
		return;

	temp = strchr(arg, ' ');

	if (temp == NULL) {
		printf("\n");
		b_echostr_s();
		printf("No message to send.\n");
		return;
	}
	if (strlen(temp + 1) == 0) {
		printf("\nNo message to send.\n");
		return;
	}
	if (conn->netspeak_filter)
		msg = undo_netspeak(temp + 1);
	else
		msg = temp + 1;

	sn = malloc(temp - arg + 1);
	sn[temp - arg] = 0;
	strncpy(sn, arg, temp - arg);
	fullmsg_len =
		strlen(msg) + strlen(SEND_FORMAT_BEGIN) + strlen(SEND_FORMAT_END) +
		1;
	fullmsg = malloc(fullmsg_len + 1);
	snprintf(fullmsg, fullmsg_len, "%s%s%s", SEND_FORMAT_BEGIN, msg,
		 SEND_FORMAT_END);

	int otr_message = otr_imcomm_im_send_message(sn, fullmsg);

	free(fullmsg);
	eraseline();

	if (conn->timestamps) {
		addts();
		putchar(' ');
		offset = 13;
	} else {
		offset = 2;
	}

	offset += strlen(sn) + 2;
	set_color(COLOR_OUTGOING_IM);
	if (otr_message)
		printf("->%s", sn);
	else
		offset += 5;
		printf("(otr)->%s", sn);
	set_color(0);
	printf(": ");
	msg_strip = strip_html(msg);
	wordwrap_print(msg_strip, offset);
	free(msg_strip);

	if (conn->lastsn != NULL)
		free(conn->lastsn);

	conn->lastsn = strdup(sn);
	log_event(EVENT_IMSEND, sn, msg);
	free(sn);

	if (conn->netspeak_filter)
		free(msg);
}

/* PROTO */
void
input_paste(char *arg)
{
	char           *sn, *sendbuf, *pbuf;
	size_t          sendbuflen;
	ssize_t         pbuflen;
	uint16_t        maxmsgsize = imcomm_get_max_message_size(conn->conn);

	if (conn->conn == NULL)
		return;

	sn = arg;

	if (sn[0] == 0) {
		printf("\n");
		b_echostr_s();
		printf("No recipient specified.\n");
		return;
	}
	printf("\n");
	b_echostr_s();
	printf("Pasting to %s.\n", sn);
	b_echostr_s();
	printf("Enter '.' on a line by itself to send," " Ctrl-X to cancel.");

	pbuf = malloc(maxmsgsize + 1);
	sendbuf = malloc(maxmsgsize + 1);
	memset(sendbuf, 0, maxmsgsize + 1);

	sendbuflen = 0;

	/*
         * using fgets() produced strange results. had to resort to writing
         * my own input stuff.
         */

	while (1) {
		printf("\n-> ");
		fflush(stdout);

		pbuflen = bsf_getline(pbuf, maxmsgsize - 5);

		if (pbuflen == -1) {
			free(pbuf);
			free(sendbuf);
			printf("\n");
			return;
		}
		if (pbuflen == 1 && pbuf[0] == '.') {
			if (sendbuflen > 0) {
				otr_imcomm_im_send_message(sn, sendbuf);
			}
			break;
		}
		/*
	         * +4 is for "<br>"
	         */

		if (sendbuflen + pbuflen + 4 > maxmsgsize) {
			otr_imcomm_im_send_message(sn, sendbuf);
			memset(sendbuf, 0, maxmsgsize + 1);

			memcpy(sendbuf, pbuf, pbuflen);
			sendbuflen = pbuflen;
		} else {
#ifdef __OpenBSD__
			if (sendbuflen != 0) {
				strlcat(sendbuf, "<br>", maxmsgsize + 1);
				pbuflen += 4;
			}
			strlcat(sendbuf, pbuf, maxmsgsize + 1);
#else
			if (sendbuflen != 0) {
				strcat(sendbuf, "<br>");
				pbuflen += 4;
			}
			strcat(sendbuf, pbuf);
#endif
			sendbuflen += pbuflen;
		}
	}

	printf("\n");

	free(sendbuf);
	free(pbuf);
}

/* PROTO */
void
input_show_buddies(char *arg)
{
	printf("\n");
	b_echostr_s();
	addts();
	printf(" %d buddies online:\n", conn->buddiesonline);

	if (arg[0] == 'f')
		display_buddylist();
	else
		display_buddylist_sorted();

}

/* PROTO */
void
input_show_help(char *arg)
{
	printf("\nbsflite commands:\n");
	printf("   h............: what you see right now\n");
	printf("   w............: show buddy list\n");
	printf("   ws...........: show buddy list, sorted (default)\n");
	printf("   wf...........: show buddy list, unsorted\n");
	printf("   l<sn>........: show last 20 lines from <sn>\n");
	printf("   L............: show last 20 lines from last buddy\n");
	printf("   m<sn> <msg>..: send msg to sn\n");
	printf("   r<msg>.......: reply to the last msg received/sent\n");
	printf("   p<sn>........: paste to sn\n");
	printf("   a<sn>........: add buddy\n");
	printf("   d<sn>........: delete buddy\n");
	printf("   g<msg>.......: set away message\n");
	printf("   g............: unset away message\n");
	printf("   G............: display pre-defined away messages\n");
	printf("   G<num>.......: set pre-defined away message\n");
	printf("   i<sn>........: get profile and away message\n");
	printf("   A<sn>........: get away message\n");
	printf("   P<sn>........: get profile\n");
	printf("   R![sn].......: manually reconnect [as [sn]]\n");
	printf("   z!...........: reload profile and away messages\n");
	printf("   q!...........: quit\n");
}

/* PROTO */
void
input_show_log(char *arg)
{
	/*
         * A single 'l' is too short to be a screen name...
         */

	if (arg[0] == 'l' && arg[1] == 0) {
		if (conn->lastsn != NULL) {
			show_log(20, conn->lastsn);
		} else {
			printf("\n");
		}
	} else {
		show_log(20, arg);
	}
}

/* PROTO */
void
input_add_buddy(char *arg)
{
	if (conn->conn == NULL)
		return;

	imcomm_im_add_buddy(conn->conn, arg);

	printf("\n");
	b_echostr_s();
	printf("Added buddy %s.\n", arg);
}

/* PROTO */
void
input_delete_buddy(char *arg)
{
	if (conn->conn == NULL)
		return;

	imcomm_im_remove_buddy(conn->conn, arg);

	printf("\n");
	b_echostr_s();
	printf("Removed buddy %s.\n", arg);

	remove_from_list(arg);
}

/* PROTO */
void
input_set_invisible(char *arg)
{
	if (conn->isinvisible) {
		imcomm_set_invisible(conn->conn, 0);
		conn->isinvisible = 0;
		printf("\n");
		b_echostr_s();
		printf("You are now visible.\n");
	} else {
		imcomm_set_invisible(conn->conn, 1);
		conn->isinvisible = 1;
		printf("\n");
		b_echostr_s();
		printf("You are now invisible.\n");
	}
}

/* PROTO */
void
input_set_away(char *arg)
{
	if (conn->conn == NULL)
		return;

	if (conn->isaway) {
		conn->isaway = 0;
		free(conn->awaymsg);
		conn->awaymsg = NULL;

		imcomm_set_unaway(conn->conn);
		printf("\n");
		b_echostr_s();
		printf("You are no longer away.\n");
		return;
	} else {
		printf("\n");
		b_echostr_s();
		printf("You are now away: %s\n", arg);
		conn->isaway = 1;
		imcomm_set_away(conn->conn, arg);
		conn->awaymsg = strdup(arg);
		return;
	}
}

/* PROTO */
void
input_get_info(char *arg)
{
	if (conn->conn == NULL)
		return;

	imcomm_request_awayprofile(conn->conn, arg);
	printf("\n");
}

/* PROTO */
void
input_get_away(char *arg)
{
	if (conn->conn == NULL)
		return;

	imcomm_request_awaymsg(conn->conn, arg);
	printf("\n");
}

/* PROTO */
void
input_get_profile(char *arg)
{
	if (conn->conn == NULL)
		return;

	imcomm_request_profile(conn->conn, arg);
	printf("\n");
}

/* PROTO */
void
input_set_predefaway(char *arg)
{
	int             msgnum = 0, count;
	struct AwayMessages *tv;

	if (conn->conn == NULL)
		return;

	if (conn->awaymsgs == NULL) {
		printf("\n");
		b_echostr_s();
		printf("No away messages defined.\n");
		return;
	}
	if (arg[0] == 0) {
		printf("\n");
		b_echostr_s();
		printf("Defined messages:");
		for (tv = conn->awaymsgs, count = 0; tv != NULL;
		     tv = tv->next, count++) {
			printf("\n");
			b_echostr_s();
			printf("[%02d] \"%s\"", count, tv->message);
		}
	} else {
		msgnum = atoi(arg);
		for (tv = conn->awaymsgs, count = 0; tv != NULL;
		     tv = tv->next, count++) {
			if (count == msgnum) {
				printf("\n");
				b_echostr_s();
				printf("You are now away: %s", tv->message);
				conn->isaway = 1;
				imcomm_set_away(conn->conn, tv->message);
				conn->awaymsg = strdup(tv->message);
				break;
			}
		}
	}

	printf("\n");
}

/* PROTO */
ssize_t
bsf_getline(char *str, int size)
{
	size_t          count = 0;
	char            ch;

	while (count < size - 1) {

#if defined(__MINGW32__) || defined(__DJGPP__)
		ch = getch();
#else
		read(fileno(stdin), &ch, 1);
#endif
		if (ch == '\n' || ch == '\r') {
			break;
		} else if (ch == 24) {
			/* Ctrl-X */
			return -1;
		} else if (ch == '\b' || ch == 127 || ch == 4) {
			count--;
			printf("\b \b");
			fflush(stdout);
		} else {
			str[count] = ch;
			count++;
			putchar(ch);
			fflush(stdout);
		}
	}

	str[count] = 0;

	return count;
}

/* PROTO */
void
input_reconnect(char *arg)
{
	struct ConnPtr *temp, *trav;

	eraseline();
	b_echostr_s();

	if (conn->timestamps) {
		addts();
		putchar(' ');
	}
	if (arg[0] != '\0') {
		if (conn->username) {
			free(conn->username);
			conn->username = NULL;
		}
		if (conn->password) {
			free(conn->password);
			conn->password = NULL;
		}
		printf("Reconnecting as %s...\n", arg);

		conn->username = strdup(arg);
		b_getpassword();
	} else {
		printf("Reconnecting...\n");
	}

	if (conn->conn != NULL) {

		delete_buddylist(buddylist);

		buddylist = NULL;
		conn->buddiesonline = 0;

		if (conn->clist->conn == conn->conn) {
			temp = conn->clist;
			conn->clist = conn->clist->next;
			if (temp->username)
				free(temp->username);

			free(temp);
		} else {
			for (trav = conn->clist; trav->next;) {
				if (trav->next->conn == conn->conn) {
					temp = trav->next;
					trav->next = trav->next->next;

					if (temp->username)
						free(temp->username);

					free(temp);
				}
			}
		}

		imcomm_delete_handle_now(conn->conn);

		conn->conn = NULL;
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

	if (conn->isaway)
		imcomm_set_away(conn->conn, conn->awaymsg);
}

void input_show_otr(char *arg) {
        if (conn->conn == NULL)
                return;

        printf("\n");
        check_key_gen_state();
}

void input_stop_otr(char *arg) {
        if (conn->conn == NULL)
                return;

        printf("\n");
	b_echostr_s();
	struct BuddyList *buddy = find_buddy(arg);
	if (buddy) {
		if (buddy->otr == 1) {
			otrl_message_disconnect_all_instances(userstate, &ui_ops, NULL,
			conn->username, otr_proto, buddy->sn);
			buddy->otr = 0;
			buddy->otr_context = NULL;
			printf("[OTR] Ending OTR session with %s\n", buddy->sn);
		} else {
			printf("[OTR] No OTR session found with %s\n", buddy->sn);
		}
	} else {
		printf("[OTR] Buddy not found: %s\n", arg);
	}
}


void input_send_smp(char *arg) {
        char *msg, *temp, *sn;

	if (conn->conn == NULL)
		return;

        printf("\n");
        temp = strchr(arg, ' ');

        if (temp == NULL) {
                b_echostr_s();
                printf("No message to send.\n");
                return;
        }
        if (strlen(temp + 1) == 0) {
                b_echostr_s();
                printf("No message to send.\n");
                return;
        }
	msg = temp + 1;

        sn = malloc(temp - arg + 1);
        sn[temp - arg] = 0;
        strncpy(sn, arg, temp - arg);

	struct BuddyList *buddy = find_buddy(sn);
	if (buddy) {
		if (buddy->otr_context) {
			b_echostr_s();
			printf("[OTR] Responding to %s\n", sn);
			otrl_message_respond_smp(userstate, &ui_ops, NULL, buddy->otr_context, msg, strlen(msg));
			return;
		}
	}
	b_echostr_s();
	printf("[OTR] No OTR context found for %s\n", sn);
}
