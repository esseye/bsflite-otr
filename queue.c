/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

#ifdef MESSAGE_QUEUE
/* PROTO */
struct MessageQueue *
addToMQueue(struct MessageQueue * head, char *message, char *whom)
{
	struct MessageQueue *trav, *lptr;

	if (head == NULL) {
		head = malloc(sizeof(struct MessageQueue));
		lptr = head;
	} else {
		for (trav = head; trav->next != NULL; trav = trav->next);

		trav->next = malloc(sizeof(struct MessageQueue));
		lptr = trav->next;
	}

	lptr->next = NULL;
	lptr->message = strdup(message);
	lptr->whom = strdup(whom);
	lptr->when = (time_t) time(NULL);

	return head;
}

/* PROTO */
void
clearMQueue(struct MessageQueue * head)
{
	struct MessageQueue *trav, *tptr;

	for (trav = head; trav != NULL;) {
		tptr = trav;
		trav = trav->next;
		free(tptr->message);
		free(tptr->whom);
		free(tptr);
	}
}
#endif
