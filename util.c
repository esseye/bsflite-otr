/**
 ** bsflite - bs-free AIM client
 **
 ** (C) 2003-2007 by Claudio Leite <leitec at leitec dot org>
 **
 ** NO WARRANTY. Read the file COPYING for more details.
 **/

#include "bsf.h"

/* PROTO */
char           *
simplify_sn(const char *sn)
{
	char           *temp;
	int             x, count;

	temp = malloc(strlen(sn) + 1);
	for (x = 0, count = 0; x < strlen(sn); x++) {
		if (sn[x] == ' ')
			continue;
		temp[count] = tolower(sn[x]);
		count++;
	}

	temp = realloc(temp, count + 1);
	temp[count] = 0;
	return temp;
}

/**
 ** This differs from strip_html() in that it ignores \n and \r, and
 ** turns <BR> into \n, so that prettyprint() works properly.
 **/

/* PROTO */
char           *
strip_html(const char *message)
{
	char           *temp;
	int             x, xnot, y, count, inhtml;
	size_t          len = strlen(message);

	temp = malloc(len + 1);
	for (x = 0, count = 0, inhtml = 0; x < len; x++) {
		if (message[x] == '<' && inhtml == 0) {
			if (x + 10 < len) {

				/**
				 ** Convert links into
				 ** [http://url] link text
				 **/

				if (strncasecmp(message + x, "<a href=\"", 9) == 0) {
					xnot = x + 9;
					for (y = xnot; y < len; y++) {
						if (message[y] == '\"') {
							/*
					                 * we don't have to
					                 * worry about the
					                 * buffer size,
					                 * because it's
					                 * guaranteed to be
					                 * bigger
					                 */
							memcpy(temp + count, "[", 1);
							memcpy(temp + count + 1, message + xnot,
							       y - xnot);
							memcpy(temp + count + 1 + (y - xnot), "] ", 2);
							count += y - xnot + 3;
							x = y;
							break;
						}
					}
				}
			}
			if (x + 3 < len) {
				if (strncasecmp(message + x, "<br>", 4) == 0) {
					temp[count] = '\n';
					count++;
					x += 3;
					continue;
				}
			}
			inhtml = 1;
			continue;
		}
		if (inhtml) {
			if (message[x] == '>')
				inhtml = 0;

			continue;
		}
		if (message[x] == '&') {
			if (x + 4 < len) {
				if (strncmp(message + x, "&amp;", 5) == 0) {
					temp[count] = '&';
					count++;
					x += 4;
					continue;
				}
			}
			if (x + 5 < len) {
				if (strncmp(message + x, "&quot;", 6) == 0) {
					temp[count] = '\"';
					count++;
					x += 5;
					continue;
				}
				if (strncmp(message + x, "&nbsp;", 6) == 0) {
					temp[count] = ' ';
					count++;
					x += 5;
					continue;
				}
			}
			if (x + 3 < len) {
				if (strncmp(message + x, "&lt;", 4) == 0) {
					temp[count] = '<';
					count++;
					x += 3;
					continue;
				}
			}
			if (x + 3 < len) {
				if (strncmp(message + x, "&gt;", 4) == 0) {
					temp[count] = '>';
					count++;
					x += 3;
					continue;
				}
			}
		}
		if (message[x] == '\n' || message[x] == '\r')
			continue;
		else
			temp[count] = message[x];
		count++;
	}

	temp = realloc(temp, count + 1);
	temp[count] = 0;
	return temp;
}
