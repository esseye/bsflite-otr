#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* stolen from tcpdump */
#define ASCII_LINELENGTH 300
#define HEXDUMP_BYTES_PER_LINE 16
#define HEXDUMP_SHORTS_PER_LINE (HEXDUMP_BYTES_PER_LINE / 2)
#define HEXDUMP_HEXSTUFF_PER_SHORT 5 /* 4 hex digits and a space */
#define HEXDUMP_HEXSTUFF_PER_LINE \
        (HEXDUMP_HEXSTUFF_PER_SHORT * HEXDUMP_SHORTS_PER_LINE)

void printhex(unsigned char *cp, int length)
{
    u_int i, oset = 0;
    int s1, s2;
    int nshorts;
    char hexstuff[HEXDUMP_SHORTS_PER_LINE*HEXDUMP_HEXSTUFF_PER_SHORT+1], *hsp;
    char asciistuff[ASCII_LINELENGTH+1], *asp;

    nshorts = length / sizeof(u_short);
    i = 0;
    hsp = hexstuff; asp = asciistuff;
    while (--nshorts >= 0) {
    s1 = *cp++;
    s2 = *cp++;
    snprintf(hsp, sizeof(hexstuff) - (hsp - hexstuff),
         " %02x%02x", s1, s2);
    hsp += HEXDUMP_HEXSTUFF_PER_SHORT;
    *(asp++) = (isgraph(s1) ? s1 : '.');
    *(asp++) = (isgraph(s2) ? s2 : '.');
    i++;
    if (i >= HEXDUMP_SHORTS_PER_LINE) {
        *hsp = *asp = '\0';
        printf("0x%04x: %-*s  %s\n",
           oset, HEXDUMP_HEXSTUFF_PER_LINE,
           hexstuff, asciistuff);
        i = 0; hsp = hexstuff; asp = asciistuff;
        oset += HEXDUMP_BYTES_PER_LINE;
    }
    }
    if (length & 1) {
    s1 = *cp++;
    snprintf(hsp, sizeof(hexstuff) - (hsp - hexstuff),
         " %02x", s1);
    hsp += 3;
    *(asp++) = (isgraph(s1) ? s1 : '.');
    ++i;
    }
    if (i > 0) {
    *hsp = *asp = '\0';
    printf("0x%04x: %-*s  %s\n",
           oset, HEXDUMP_HEXSTUFF_PER_LINE,
           hexstuff, asciistuff);
    }
}
