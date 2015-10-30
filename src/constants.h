#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdio.h>

#define USER_ID_INVALID -1

typedef unsigned char byte;

#define DEBUG 2

#define debugv(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "[DD] %s:%d:%s():  " fmt, __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while(0)

#define errv(fmt, ...) \
    do { fprintf(stderr, "[EE] %s:%d:%s():  " fmt, __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while(0)

#endif
