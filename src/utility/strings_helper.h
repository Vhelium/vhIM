#ifndef STRING_HELPER_H
#define STRING_HELPER_H

#include <stdbool.h>

/* checks wheter the given string is a number or not */
bool is_decimal_number(char *in);

/* returns the next word of a string in a newly allocated string.
 * @in: adress to the input string
 * @out: adress to the output string
 * returns: length of the word
 */
int next_word(char **in, char **out);

int read_line(char str[], int n);

void bytes_to_string(const char *in, int in_len, char *out);

void ubytes_to_string(const unsigned char *in, int in_len, char *out);

#endif
