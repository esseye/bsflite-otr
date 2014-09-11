/*
 * Optional ANSI colors for BSFlite
 *
 * These are in the format [B][BG][FG], where B is 1 or 0 - bold or not bold, BG
 * is background color from 0 to 7 (ANSI) FG is foreground color from 0 to 7
 * (ANSI)
 *
 * For example, for a bold white with black background, you would use 107.
 *
 * Note that if COLOR_DARKBG isn't defined, background colors will not be set.
 *
 * Color reference: 0    black 1    red 2    green 3    brown/yellow 4    blue 5
 * magenta 6    cyan 7    white
 */

#ifdef COLOR_DARKBG
#define COLOR_INCOMING_IM 101
#define COLOR_OUTGOING_IM 106
#define COLOR_AUTORESPONSE 104

#define COLOR_ECHOSTR 102
#define COLOR_PROMPT 000
#define COLOR_TIMESTAMP 100
#define COLOR_PROFILE_ECHOSTR 101
#define COLOR_PROFILE 000

#define COLOR_BUDDY_SIGNON 107
#define COLOR_BUDDY_SIGNOFF 107
#define COLOR_BUDDY_AWAY 107
#define COLOR_BUDDY_IDLE 107

#define COLOR_WAITING 107

#define COLOR_BUDDYLIST_ECHOSTR 100
#define COLOR_BUDDYLIST_ONLINE 107
#define COLOR_BUDDYLIST_IDLE 000
#define COLOR_BUDDYLIST_AWAY 103
#define COLOR_BUDDYLIST_GONE 000

#else

#define COLOR_INCOMING_IM 101
#define COLOR_OUTGOING_IM 006
#define COLOR_AUTORESPONSE 004

#define COLOR_ECHOSTR 005
#define COLOR_PROFILE_ECHOSTR 001
#define COLOR_PROFILE 000
#define COLOR_PROMPT 000
#define COLOR_TIMESTAMP 000

#define COLOR_BUDDY_SIGNON 006
#define COLOR_BUDDY_SIGNOFF 006
#define COLOR_BUDDY_AWAY 006
#define COLOR_BUDDY_IDLE 006

#define COLOR_WAITING 100

#define COLOR_BUDDYLIST_ONLINE 001
#define COLOR_BUDDYLIST_IDLE 002
#define COLOR_BUDDYLIST_AWAY 003
#define COLOR_BUDDYLIST_GONE 004

#endif
