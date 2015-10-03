#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdio.h>

typedef unsigned char byte;

#define DEBUG 0

#define debugv(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "[DD] %s:%d:%s():  " fmt, __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while(0)

#define errv(fmt, ...) \
    do { fprintf(stderr, "[EE] %s:%d:%s():  " fmt, __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while(0)

#endif
