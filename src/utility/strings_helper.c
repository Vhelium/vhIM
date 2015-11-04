#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../constants.h"
#include "strings_helper.h"

bool is_decimal_number(char *in)
{
    while(*in)
        if (isdigit(*in++) == 0)
            return 0;
    return 1;
}

int next_word(char **in, char **out)
{
    char *s = *in;
    while (*s == ' ') /* ignore preceeding whitespaces */
        s++;
    char *p = s;
    while (*p != ' ' && *p != '\0')
        ++p;
    int len = p - s;
    if (len > 0) {
        *out = malloc(sizeof(char) * (len+1));
        memcpy(*out, s, len);
        *(*out+len) = '\0';

        *in = p; // adjust input line to 'delete' the first word
    }

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

void bytes_to_string(const char *in, int in_len, char *out)
{
    int i;
    for(i=0; i<in_len; ++i) {
        sprintf(out, "%0.2X", in[i]);
        out+=2;
    }
    *out = '\0';
}

void ubytes_to_string(const unsigned char *in, int in_len, char *out)
{
    int i;
    for(i=0; i<in_len; ++i) {
        sprintf(out, "%0.2X", in[i]);
        out+=2;
    }
    *out = '\0';
}
