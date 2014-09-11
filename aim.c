/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

extern struct Conn *conn;
extern struct BuddyList *buddylist;
extern struct Waiting *waiting;

extern OtrlUserState userstate;
extern OtrlMessageAppOps ui_ops;
extern char *otr_proto;

/* PROTO */
void
getmessage(void *c, const char *who, const int automessage, const char *message)
{
	const char	*msgin = message;
	char           *msg = NULL, *tempmsg = NULL;

	char           *sname;
	struct Waiting *wtemp, *wptr = NULL;
	int             offset, foundWaiting = 0;

	int otr_message = 0;
	if (conn->otr) {
		struct BuddyList	*buddy = NULL;
		buddy = find_buddy(who);

		if (buddy) {
			if (buddy->otr != -1) {
				char *newmsg;
				int ret = otrl_message_receiving(userstate, &ui_ops, NULL, conn->username,
								otr_proto, buddy->sn, msgin, &newmsg, NULL,
								NULL, NULL, NULL);
				if (ret) {
#ifdef DEBUG
					b_echostr_s();
					printf("[OTR] Debug: internal msg %s\n", msgin);
#endif
					return;
				} else {
					if (newmsg) {
						msgin = strdup(newmsg);
						otrl_message_free(newmsg);
						otr_message = 1;
					}
				}
			}
		}
	}

	tempmsg = strip_html(msgin);

	
	if (tempmsg == NULL)
		return;
	if (strlen(tempmsg) == 0) {
		free(tempmsg);
		return;
	}
	if (conn->netspeak_filter) {
		msg = undo_netspeak(tempmsg);
		free(tempmsg);
	} else {
		msg = tempmsg;
	}

	if (msg == NULL)
		return;

	if (strlen(msg) == 0) {
		free(msg);
		return;
	}
	if (conn->istyping == 0) {
		if (conn->lastsn != NULL)
			free(conn->lastsn);
		conn->lastsn = simplify_sn(who);
	}
	sname = simplify_sn(who);

	if (waiting == NULL) {
		waiting = malloc(sizeof(struct Waiting));
		wptr = waiting;
	} else {
		for (wtemp = waiting; wtemp != NULL; wtemp = wtemp->next)
			if (imcomm_compare_nicks(c, wtemp->sn, who)) {
				foundWaiting = 1;
				wptr = wtemp;
				break;
			}
		if (!foundWaiting) {
			for (wtemp = waiting; wtemp->next != NULL;
			     wtemp = wtemp->next);
			wtemp->next = malloc(sizeof(struct Waiting));
			wptr = wtemp->next;
		}
	}

	if (!foundWaiting) {
		wptr->sn = strdup(who);
		wptr->next = NULL;

		if (conn->isaway && !automessage) {
			if ((conn->respond_idle_only && conn->isidle)
			    || (!conn->respond_idle_only)) {
				imcomm_im_send_message(c, who, conn->awaymsg, 1);
				eraseline();
				b_echostr_s();

				if (conn->timestamps) {
					addts();
					putchar(' ');
				}
				printf("Sent auto-response to %s.\n", who);
				show_prompt();
			}
		}
	}
#ifdef MESSAGE_QUEUE
	if (conn->isaway)
		wptr->mqueue = addToMQueue(wptr->mqueue, msg, who);
#endif				/* MESSAGE_QUEUE */


	eraseline();

	if (conn->bell_on_incoming)
		putchar('\a');

	if (conn->timestamps) {
		addts();
		putchar(' ');
		offset = 11;
	} else {
		offset = 0;
	}

	offset += strlen(who) + 2;
	if (automessage) {
		set_color(COLOR_AUTORESPONSE);
		printf("*AUTO RESPONSE* ");
		set_color(0);
		offset += 16;
	}
	set_color(COLOR_INCOMING_IM);
	if (!otr_message)
		printf("%s", who);
	else
		offset += 5;
		printf("(otr)%s", who);
	set_color(0);
	printf(": ");
	wordwrap_print(msg, offset);
	if (automessage)
		log_event(EVENT_IM_AUTORESPONSE, sname, msg);
	else
		log_event(EVENT_IM, sname, msg);

	free(msg);
	free(sname);
	show_prompt();
}

struct BuddyList *find_buddy(const char *who) {
	struct BuddyList *trav;
	char           *sn = simplify_sn(who);
        trav = buddylist;

        while (trav != NULL) {
                if (strcmp(trav->sn, sn) == 0) {
        		free(sn);
                        return(trav);
                }
                trav = trav->next;
        }

        free(sn);

        /*
         * in case for whatever reason the buddy isn't in the list (shouldn't
         * happen)
         */
        if (trav == NULL) {
                return(NULL);
        }
}

/* PROTO */
void
buddy_idle(void *c, const char *who, long idletime)
{
	struct BuddyList *trav;
	int             changed = 1;
	char           *sn = simplify_sn(who);

	trav = buddylist;

	while (trav != NULL) {
		if (strcmp(trav->sn, sn) == 0) {
			trav->idletime = idletime;

			if (idletime >= 10) {
				if (trav->idle)
					changed = 0;
				trav->idle = 1;
			} else {
				if (trav->idle == 0)
					changed = 0;
				trav->idle = 0;
			}
			break;
		}
		trav = trav->next;
	}

	free(sn);

	/*
         * in case for whatever reason the buddy isn't in the list (shouldn't
         * happen)
         */
	if (trav == NULL) {
		return;
	}
	if (!changed) {
		return;
	}
	if (conn->squelchidle)
		return;

	eraseline();
	b_echostr();

	if (conn->timestamps) {
		putchar(' ');
		addts();
	}
	set_color(COLOR_BUDDY_IDLE);
	printf(" %s ", who);
	set_color(0);

	printf("is %s idle.\n", (trav->idle ? "now" : "no longer"));
	show_prompt();
	return;
}

/* PROTO */
void
buddy_away(void *c, const char *who)
{
	struct BuddyList *trav;
	char           *sn = simplify_sn(who);

	trav = buddylist;

	while (trav != NULL) {
		if (strcmp(trav->sn, sn) == 0) {
			trav->away = 1;
			break;
		}
		trav = trav->next;
	}

	free(sn);

	if (conn->squelchaway)
		return;

	eraseline();
	b_echostr();

	if (conn->timestamps) {
		putchar(' ');
		addts();
	}
	set_color(COLOR_BUDDY_AWAY);
	printf(" %s ", who);
	set_color(0);

	printf("is away.\n");
	show_prompt();
	return;
}

/* PROTO */
void
buddy_unaway(void *c, const char *who)
{
	struct BuddyList *trav;
	char           *sn = simplify_sn(who);

	trav = buddylist;

	while (trav != NULL) {
		if (strcmp(sn, trav->sn) == 0) {
			trav->away = 0;
			break;
		}
		trav = trav->next;
	}

	free(sn);

	if (conn->squelchaway)
		return;

	eraseline();
	b_echostr();

	if (conn->timestamps) {
		putchar(' ');
		addts();
	}
	set_color(COLOR_BUDDY_AWAY);
	printf(" %s ", who);
	set_color(0);
	printf("is no longer away.\n");
	show_prompt();
	return;
}

/* PROTO */
void
buddy_online(void *c, const char *who)
{
	struct BuddyList *trav, *newbuddy;
	char           *sname;

	trav = buddylist;

	sname = simplify_sn(who);
	if (buddylist == NULL) {
		buddylist = malloc(sizeof(struct BuddyList));
		newbuddy = buddylist;
		newbuddy->prev = NULL;
		newbuddy->next = NULL;
	} else {
		for (trav = buddylist; trav != NULL; trav = trav->next) {
			if (strcmp(sname, trav->sn) < 0)
				break;
		}

		newbuddy = malloc(sizeof(struct BuddyList));

		if (trav == NULL) {	/* if it's the last entry */
			for (trav = buddylist; trav->next != NULL; trav = trav->next);
			trav->next = newbuddy;
			newbuddy->prev = trav;
			newbuddy->next = NULL;
		} else {
			if (trav == buddylist) {
				buddylist->prev = newbuddy;
				newbuddy->prev = NULL;
				newbuddy->next = buddylist;
				buddylist = newbuddy;
			} else {
				trav->prev->next = newbuddy;
				newbuddy->prev = trav->prev;
				newbuddy->next = trav;
				trav->prev = newbuddy;
			}
		}

	}

	newbuddy->sn = strdup(sname);
	newbuddy->formattedsn = strdup(who);
	newbuddy->away = 0;
	newbuddy->idle = 0;
	newbuddy->otr = 0;
	newbuddy->otr_context = otrl_context_find(userstate, newbuddy->sn,
					conn->username, otr_proto,
					OTRL_INSTAG_BEST, 1, 0,
					NULL, NULL);

	conn->buddiesonline++;

	if (conn->squelchconnect) {
		free(sname);
		return;
	}
	eraseline();
	b_echostr();

	if (conn->timestamps) {
		putchar(' ');
		addts();
	}
	set_color(COLOR_BUDDY_SIGNON);
	printf(" %s ", who);
	set_color(0);
	printf("is now online.\n");
	log_event(EVENT_SIGNON, sname, NULL);
	free(sname);
	show_prompt();
}

/* PROTO */
void
remove_from_list(const char *who)
{
	char           *sname;
	struct BuddyList *trav;
	sname = simplify_sn(who);
	trav = buddylist;
	while (trav != NULL) {
		if (strcmp(trav->sn, sname) == 0) {
			conn->buddiesonline--;
			if (trav->prev == NULL) {
				buddylist = buddylist->next;
				if (buddylist != NULL)
					buddylist->prev = NULL;
				free(trav->sn);
				free(trav->formattedsn);
				free(trav);
				break;
			} else {
				trav->prev->next = trav->next;
				if (trav->next != NULL)
					trav->next->prev = trav->prev;
				free(trav->sn);
				free(trav->formattedsn);
				free(trav);
				break;
			}
		}
		trav = trav->next;
	}

	free(sname);
}

/* PROTO */
void
buddy_offline(void *c, const char *who)
{
	char           *sname;
	int             found = 0;
	struct BuddyList *trav;

	sname = simplify_sn(who);

	trav = buddylist;
	while (trav != NULL) {
		if (strcmp(trav->sn, sname) == 0) {
			found = 1;

			if (trav->otr == 1) {
				otrl_message_disconnect_all_instances(userstate, &ui_ops, NULL,
      	                	conn->username, otr_proto, trav->sn);
             			trav->otr = 0;
                        	trav->otr_context = NULL;
                        	printf("[OTR] Ending OTR session with %s\n", trav->sn);
			}

			if (trav->prev == NULL) {
				buddylist = buddylist->next;
				if (buddylist != NULL)
					buddylist->prev = NULL;
				free(trav->sn);
				free(trav->formattedsn);
				free(trav);
				break;
			} else {
				trav->prev->next = trav->next;
				if (trav->next != NULL)
					trav->next->prev = trav->prev;
				free(trav->sn);
				free(trav->formattedsn);
				free(trav);
				break;
			}
		}
		trav = trav->next;
	}

	if (!found) {
		free(sname);
		return;
	}
	conn->buddiesonline--;

	if (conn->squelchconnect) {
		free(sname);
		return;
	}
	eraseline();
	b_echostr();

	if (conn->timestamps) {
		putchar(' ');
		addts();
	}
	set_color(COLOR_BUDDY_SIGNOFF);
	printf(" %s ", who);
	set_color(0);

	printf("has signed off.\n");
	log_event(EVENT_SIGNOFF, sname, NULL);
	free(sname);
	show_prompt();
}

/* PROTO */
void
buddy_profile(void *c, const char *who, const char *msg)
{
	char           *ptext;
	long            days, hours, minutes;
	struct BuddyList *trav;
	char           *sname;
#ifdef DUMP_PROFILE
	FILE           *profile_dump;
#endif

	sname = simplify_sn(who);
	eraseline();

	for (trav = buddylist; trav; trav = trav->next) {
		if (strcmp(trav->sn, sname) == 0) {
			if (trav->idle) {
				b_echostr_s();
				printf("[%s] Idle: ", who);
				days = trav->idletime / 1440;
				hours = (trav->idletime % 1440) / 60;
				minutes = trav->idletime % 60;
				if (days > 0)
					printf("%ld days, ", days);
				if (hours > 0)
					printf("%ld hour%s, ", hours, (hours != 1 ? "s" : ""));

				printf("%ld minute%s\n", minutes,
				       (minutes != 1 ? "s" : ""));
				break;
			}
		}
	}

	b_echostr_s();
	printf("[%s] Info:\n", who);

	free(sname);

	ptext = strip_html(msg);
	prettyprint_echostr(ptext, PROFILE_ECHOSTR);

	free(ptext);

#ifdef DUMP_PROFILE
	if ((profile_dump = fopen(PROFILE_DUMP_PATH, "w")) != NULL) {
		fprintf(profile_dump, "%s", msg);
		fclose(profile_dump);
	}
#endif

	show_prompt();
}

/* PROTO */
void
buddy_awaymsg(void *c, const char *who, const char *msg)
{
	char           *ptext;

	eraseline();
	b_echostr_s();
	printf("[%s] Away: ", who);
	ptext = strip_html(msg);
	prettyprint(ptext, 12 + strlen(who));

	if (ptext[0] == 0)
		printf("\n");

	free(ptext);
	show_prompt();
}

/* PROTO */
void
delete_buddylist(struct BuddyList * buddylist)
{
	struct BuddyList *tr, *tmp;

	for (tr = buddylist; tr;) {
		if (tr->sn)
			free(tr->sn);
		if (tr->formattedsn)
			free(tr->formattedsn);

		tmp = tr;
		tr = tr->next;

		free(tmp);
	}
}
