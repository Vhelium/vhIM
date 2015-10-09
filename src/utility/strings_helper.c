#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../constants.h"
#include "strings_helper.h"

int next_word(char **in, char **out)
{
    char *s = *in;
    while (*s == ' ') /* ignore preceeding whitespaces */
        s++;
    char *p = s;
    while (*p != ' ' && *p != '\0')
        ++p;
    int len = p - s;
    *out = malloc(sizeof(char) * (len+1));
    memcpy(*out, s, len);
    *(*out+len) = '\0';

    *in = p; // adjust input line to 'delete' the first word

    return len;
}

int read_line(char str[], int n)
{
    int ch, i = 0;
    while(isspace(ch = getchar()))
                    ;
    while(ch != '\n' && ch != EOF) {
        if(i < n)
            str[i++] = ch;
        ch = getchar();
    }
    str[i] = '\0';
    return i;
}
