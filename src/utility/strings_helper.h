#ifndef STRING_HELPER_H
#define STRING_HELPER_H

/* returns the next word of a string in a newly allocated string.
 * @in: adress to the input string
 * @out: adress to the output string
 * returns: length of the word
 */
int next_word(char **in, char **out);

int read_line(char str[], int n);

#endif
