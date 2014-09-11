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
void           *
create_new_connection(void)
{
	struct ConnPtr *temp, *trav;

	temp = malloc(sizeof(struct ConnPtr));
	temp->next = NULL;
	temp->username = NULL;

	temp->conn = (void *) imcomm_create_handle();
	set_profile(temp->conn);
	imcomm_register_callback(temp->conn, IMCOMM_IM_INCOMING, getmessage);
	imcomm_register_callback(temp->conn, IMCOMM_IM_SIGNON, buddy_online);
	imcomm_register_callback(temp->conn, IMCOMM_IM_SIGNOFF, buddy_offline);
	imcomm_register_callback(temp->conn, IMCOMM_IM_BUDDYAWAY, buddy_away);
	imcomm_register_callback(temp->conn, IMCOMM_IM_BUDDYUNAWAY,
				 buddy_unaway);
	imcomm_register_callback(temp->conn, IMCOMM_IM_IDLEINFO, buddy_idle);
	imcomm_register_callback(temp->conn, IMCOMM_IM_PROFILE, buddy_profile);
	imcomm_register_callback(temp->conn, IMCOMM_IM_AWAYMSG, buddy_awaymsg);
	imcomm_register_callback(temp->conn, IMCOMM_ERROR, error_callback);
	imcomm_register_callback(temp->conn, IMCOMM_FORMATTED_SN,
				 receive_formatted_sn);
	imcomm_register_callback(temp->conn, IMCOMM_HANDLE_DELETED,
				 handle_deleted);

	if (conn->clist == NULL) {
		conn->clist = temp;
	} else {
		for (trav = conn->clist; trav->next != NULL; trav = trav->next);
		trav->next = temp;
	}

	return temp;
}

/* PROTO */
void
receive_formatted_sn(void *handle, char *sn)
{
	struct ConnPtr *trav;

	for (trav = conn->clist; trav != NULL; trav = trav->next) {
		if (handle == trav->conn) {
			if (trav->username == NULL) {
				trav->username = strdup(sn);
			} else {
				free(trav->username);
				trav->username = strdup(sn);
			}
		}
	}

	if (conn->set_window_title) {
		char            buf[257];

		snprintf(buf, sizeof(buf) - 1, "bsflite: %s", sn);

#ifdef __MINGW32__
		SetConsoleTitle(buf);
#else
		set_title(buf);
#endif
	}
}
