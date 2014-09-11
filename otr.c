#include <stdio.h>
#include <string.h>
#include <gcrypt.h>
#include <pthread.h>
#include <libotr/proto.h>
#include <libotr/userstate.h>
#include <libotr/message.h>
#include <libotr/privkey.h>

#include "bsf.h"
 
OtrlUserState userstate;
OtrlMessageAppOps ui_ops;

extern struct Conn *conn;
extern struct BuddyList *buddylist;

char *otr_proto = "prpl-oscar";

int key_gen_state = 0;
void *key_gen_newkey;
gcry_error_t key_gen_error = 0;

static pthread_t keygen_thread;

static OtrlPolicy otr_policy_cb(void *data, ConnContext *context) {
	return OTRL_POLICY_DEFAULT;
}

static void otr_inject_msg_cb(void *data, const char *accountname, const char *protocol,
				const char *recipient, const char *message) {
	char *prefix = "<HTML><BODY>";
	char *suffix = "</BODY></HTML>";
	size_t len = strlen(prefix) + strlen(message) + strlen(suffix) + 1;
	char *final = malloc(len);
	snprintf(final, len, "%s%s%s", prefix, message, suffix);
#ifdef DEBUG
	b_echostr_s();
	printf("\n[OTR] Debug inject_msg to %s: %s\n", recipient, final);
#endif
	imcomm_im_send_message(conn->conn, recipient, final, 0);
	free(final);
	return;
}

static void write_fingerprints_cb(void *data) {
	gcry_error_t err;
	char *printfile = build_otr_file("fingerprints_");

	err = otrl_privkey_write_fingerprints(userstate, printfile);
	if (err) {
		printf("[OTR] fingerprint write failed (%s)\n", gcry_strerror(err));
	}

	free(printfile);
}

static void new_fingerprint_cb(void *data, OtrlUserState us, const char *accountname,
				const char *protocol, const char *username,
				unsigned char fingerprint[20]) {
	char human_hash[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
	otrl_privkey_hash_to_human(human_hash, fingerprint);
	b_echostr_s();
	printf("[OTR] new fingerprint received for %s: %s\n", username,
		human_hash);
}

static const char *otr_error_cb(void *data, ConnContext *context,
	OtrlErrorCode code) {
	char *msg = NULL;

	switch (code) {
	case OTRL_ERRCODE_NONE:
		break;
	case OTRL_ERRCODE_ENCRYPTION_ERROR:
		msg = strdup("Error occurred encrypting message.");
		break;
	case OTRL_ERRCODE_MSG_NOT_IN_PRIVATE:
		if (context) {
			msg = strdup("You sent encrypted data which was unexpected");
		}
		break;
	case OTRL_ERRCODE_MSG_UNREADABLE:
		msg = strdup("You transmitted an unreadable encrypted message");
		break;
	case OTRL_ERRCODE_MSG_MALFORMED:
		msg = strdup("You transmitted a malformed data message.");
		break;
	}

	return msg;
}

static void otr_error_free_cb(void *data, const char *err_msg) {
	if (err_msg) {
		free((char *)err_msg);
	}
}

static void otr_smp_event_cb(void *data, OtrlSMPEvent smp_event, ConnContext *context,
				unsigned short progress_percent, char *question) {

	const char *from = context->username;

	struct BuddyList *buddy = find_buddy(from);
	switch (smp_event) {
		case OTRL_SMPEVENT_ASK_FOR_SECRET:
			b_echostr_s();
			printf("[OTR] %s wants to authenticate via secret\n", from);
			if (buddy)
				buddy->otr_context = context;
			break;
		case OTRL_SMPEVENT_ASK_FOR_ANSWER:
			b_echostr_s();
			printf("[OTR] %s wants to authenticate via question: %s\n", from, question);
			if (buddy)
				buddy->otr_context = context;
			break;
		case OTRL_SMPEVENT_IN_PROGRESS:
			b_echostr_s();
			printf("[OTR] %s replied to auth request\n", from);
			break;
		case OTRL_SMPEVENT_SUCCESS:
			b_echostr_s();
			printf("[OTR] Authentication with %s successful\n", from);
			if (buddy)
				otrl_context_set_trust(context->active_fingerprint, "verified");
				buddy->otr = 1;
				buddy->otr_context = context;
			break;
		case OTRL_SMPEVENT_ABORT:
			printf("[OTR] Authentication with %s aborted\n", from);
		case OTRL_SMPEVENT_FAILURE:
		case OTRL_SMPEVENT_CHEATED:
		case OTRL_SMPEVENT_ERROR:
			b_echostr_s();
			printf("[OTR] Authentication with %s failed\n", from);
			break;
		default:
			b_echostr_s();
			printf("[OTR] Unknown SMP event\n");
			break;
	}
}

static void otr_secure_cb(void *data, ConnContext *context) {
	int ret = otrl_context_is_fingerprint_trusted(context->active_fingerprint);
	if (ret) {
		struct BuddyList *buddy = find_buddy(context->username);
		if (buddy) {
			printf("\n");
			b_echostr_s();
			printf("[OTR] Beginning OTR session with %s\n", buddy->sn);
			buddy->otr = 1;
			buddy->otr_context = context;
		}
	} else {
		b_echostr_s();
		printf("[OTR] OTR session requested by %s\n", context->username);
	}
}

static void otr_insecure_cb(void *data, ConnContext *context) {
	struct BuddyList *buddy = find_buddy(context->username);
	if (buddy) {
		b_echostr_s();
		printf("[OTR] Ending OTR session with %s\n", buddy->sn);
		buddy->otr = 0;
		buddy->otr_context = NULL;
	}
}

static void otr_handle_msg_event_cb(void *data, OtrlMessageEvent msg_event,
		ConnContext *context, const char *message, gcry_error_t err) {
#ifdef DEBUG
	printf("\n");
	b_echostr_s();
	printf("[OTR] Debug: msg_event %d - %s (%s)\n", msg_event, message,
		gcry_strerror(err));
#endif
	return;
}

static void otr_create_instag_cb(void *data, const char *accountname, const char *protocol) {
	char *instagfile = build_otr_file("instag_");
	otrl_instag_generate(userstate, instagfile, conn->username, otr_proto);
}

static int otr_max_msg_cb(void *data, ConnContext *context) {
	return(imcomm_get_max_message_size(conn->conn));
}

static int otr_buddy_online_cb(void *data, const char *accountname, const char *protocol, const char *recipient) {
	struct BuddyList *buddy = find_buddy(recipient);
	if (buddy) {
		return(1);
	}
	return(0);
}

char *build_otr_file(const char *prefix) {
  size_t buflen = strlen(conn->configdir) + strlen(prefix) + strlen(conn->username) + 1;
  char *otr_file = malloc(buflen);
  snprintf(otr_file, buflen, "%s%s%s", conn->configdir, prefix, conn->username);
  return(otr_file);
}
 
static void *generate_key(void *newkey) {
	gcry_error_t err;

	key_gen_state = 1;

	err = otrl_privkey_generate_calculate(newkey);

	if (err != GPG_ERR_NO_ERROR) {
		key_gen_state = -1;
		goto error;
	}

	key_gen_state = 2;

error:
	key_gen_error = err;
	return NULL;
}

void finish_key_gen(void) {
	gcry_error_t err;
	char *keyfile = build_otr_file("key_");

	err = otrl_privkey_generate_finish(userstate, key_gen_newkey, keyfile);

	b_echostr_s();
	if (err) {
		printf("[OTR] key finish failed (%s)\n", gcry_strerror(err));
	} else {
		printf("[OTR] key written: %s\n", keyfile);
	}
	free(keyfile);
}

void otr_write_fingerprints(void) {
	gcry_error_t err;
	char *printfile = build_otr_file("fingerprints_");

	err = otrl_privkey_write_fingerprints(userstate, printfile);

	b_echostr_s();
	if (err) {
		printf("[OTR] fingerprint write failed (%s)\n", gcry_strerror(err));
	} else {
		printf("[OTR] fingerprints written: %s\n", printfile);
	}
	free(printfile);
}

void check_key_gen_state(void) {
	switch (key_gen_state) {
        	case 0:
			b_echostr_s();
			printf("[OTR] no key generation in progress\n");
			break;
		case 1:
			b_echostr_s();
			printf("[OTR] key generation in progress\n");
			break;
		case 2:
			b_echostr_s();
			if (key_gen_error) {
				printf("[OTR] key generation failed (%s)\n", gcry_strerror(key_gen_error));
			} else {
				printf("[OTR] key generation complete\n");
				finish_key_gen();
			}
			break;
        };
}

int init_otr() {
 
  OTRL_INIT;
 
  userstate = otrl_userstate_create();

  gcry_error_t err;

  char *keyfile = build_otr_file("key_");

  b_echostr_s();
  printf("[OTR] reading keyfile %s: ", keyfile);

  err = otrl_privkey_read(userstate, keyfile);
  if (err) {
    printf("failed (%s)\n", gcry_strerror(err));
    b_echostr_s();
    printf("[OTR] generating new private key\n");
    err = otrl_privkey_generate_start(userstate, conn->username, otr_proto, &key_gen_newkey);
    if (!err) {
      int ret = pthread_create(&keygen_thread, NULL, generate_key, key_gen_newkey);
      if (ret < 0) {
        b_echostr_s();
        printf("[OTR] Key generation failed. Thread failure: %s", strerror(errno));
      }
    } else {
      b_echostr_s();
      printf("[OTR] key generation setup failed (%s)\n", gcry_strerror(err));
    }
  } else {
    printf("success\n");
  }

  free(keyfile);

  char *printfile = build_otr_file("fingerprints_");

  b_echostr_s();
  printf("[OTR] reading fingerprints %s: ", printfile);

  err = otrl_privkey_read_fingerprints(userstate, printfile, NULL, NULL);
  if (err) {
    printf("failed (%s)\n", gcry_strerror(err));
  } else {
    printf("success\n");
  }

  free(printfile);
  
  char *instagfile = build_otr_file("instag_");

  b_echostr_s();
  printf("[OTR] reading instags %s: ", instagfile);
  otrl_instag_generate(userstate, instagfile, conn->username, otr_proto);

  err = otrl_instag_read(userstate, instagfile);
  if (err) {
    printf("failed (%s)\n", gcry_strerror(err));
  } else {
    printf("success\n");
  }

  free(instagfile);

  return 0;
}

void otr_cleanup() {
        struct BuddyList *tr;

        for (tr = buddylist; tr;) {
		otrl_message_disconnect_all_instances(userstate, &ui_ops, NULL, conn->username,
							otr_proto, tr->sn);
		tr = tr->next;
	}

	otr_write_fingerprints();
	otrl_userstate_free(userstate);
}

int otr_imcomm_im_send_message(const char *sn, const char *fullmsg) {
        if (conn->otr) {
                struct BuddyList *buddy = find_buddy(sn);
                if (buddy) {
                        if (buddy->otr != -1) {
                                gcry_error_t err;
                                char *newmsg;
                                err = otrl_message_sending(userstate, &ui_ops, NULL,
                                                        conn->username, otr_proto,
                                                        buddy->sn, OTRL_INSTAG_BEST,
                                                        fullmsg, NULL, &newmsg, OTRL_FRAGMENT_SEND_SKIP,
                                                        &buddy->otr_context, NULL, NULL);
                                if (err) {
                                        b_echostr_s();
                                        printf("[OTR] error sending message: %s\n", gcry_strerror(err));
                                        return(-1);
                                } else if (newmsg != NULL) {
        				imcomm_im_send_message(conn->conn, sn, newmsg, 0);
                                        otrl_message_free(newmsg);
					return(0);
                                }
                        }
                }
        }

        imcomm_im_send_message(conn->conn, sn, fullmsg, 0);
	return(1);
}

OtrlMessageAppOps ui_ops = {
	otr_policy_cb,
	NULL, // ops_create_privkey
	otr_buddy_online_cb,
	otr_inject_msg_cb,
	NULL, // ops_up_ctx_list
        new_fingerprint_cb,
        write_fingerprints_cb,
	otr_secure_cb,
	otr_insecure_cb,
	NULL, // still_secure
	otr_max_msg_cb,
	NULL, /* account_name */
	NULL, /* account_name_free */
	NULL, /* received_symkey */
	otr_error_cb,
	otr_error_free_cb,
	NULL, /* resent_msg_prefix */
	NULL, /* resent_msg_prefix_free */
	otr_smp_event_cb,
	otr_handle_msg_event_cb,
	otr_create_instag_cb,
	NULL, /* convert_msg */
	NULL, /* convert_free */
	NULL, // ops_timer_control,
};
