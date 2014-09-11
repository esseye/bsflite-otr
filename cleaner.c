/**
 ** bsflite - bs-free AIM client
 **           ultralight version.
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

struct WordTable {
	char           *orig;
	char           *clean;
};

const struct WordTable table[] = {
	{"u", "you"},
	{"r", "are"},
	{"i", "I"},
	{"c", "see"},
	{"ic", "I see"},
	{"i'm", "I'm"},
	{"i'd", "I'd"},
	{"ive", "I've"},
	{"i've", "I've"},
	{"i'll", "I'll"},
	{"idk", "I don't know"},
	{"IDK", "I don't know"},
	{"ur", "you're"},	/* there's no way to remove the ambiguity;
				 * pick one. */
	{"ne1", "anyone"},
	{"omg", "oh my god,"},
	{"teh", "the"},
	{"cuz", "because"},
	{"hte", "the"},
	{"taht", "that"},
	{"funnay", "funny"},
	{"d00d", "dude"},
	{"dood", "dude"},
	{"liek", "like"},
	{"osmething", "something"},
	{NULL, NULL}
};

/* PROTO */
char           *
undo_netspeak(char *orig)
{
	char           *cleaned, *spcptr = orig;
	size_t          x, y, newlen, origlen = strlen(orig);
	int             found;

	newlen = origlen;

	while (1) {
		for (x = 0; table[x].orig != NULL; x++) {
			int             len = strlen(table[x].orig);
			if (strncmp(table[x].orig, spcptr, len) == 0) {
				if (spcptr[len] == ' ' || spcptr[len] == 0
				 || spcptr[len] == '.' || spcptr[len] == ','
				    || spcptr[len] == '?' || spcptr[len] == '!') {
					newlen += (ssize_t) strlen(table[x].clean) - len;
					break;
				}
			}
		}

		spcptr = strchr(spcptr, ' ');
		if (spcptr == NULL)
			break;

		spcptr++;
	}

	cleaned = malloc(newlen + 1);
	cleaned[0] = 0;

	for (x = 0; x < origlen; x++) {
		found = 0;

		if (x > 0) {
			if (orig[x - 1] != ' ') {
				strncat(cleaned, &orig[x], 1);
				continue;
			}
		}
		for (y = 0; table[y].orig != NULL; y++) {
			int             len = strlen(table[y].orig);

			if (x + len > strlen(orig))
				continue;

			if (orig[x + len] != ' ' && orig[x + len] != '.'
			    && orig[x + len] != 0 && orig[x + len] != ','
			    && orig[x + len] != '?' && orig[x + len] != '!')
				continue;

			if (strncmp(table[y].orig, orig + x, len) == 0) {
#ifdef __OpenBSD__
				strlcat(cleaned, table[y].clean, newlen + 1);
#else
				strcat(cleaned, table[y].clean);
#endif

				x += len - 1;
				found = 1;
				break;
			}
		}

		if (found)
			continue;

		strncat(cleaned, &orig[x], 1);
	}

	return cleaned;
}
