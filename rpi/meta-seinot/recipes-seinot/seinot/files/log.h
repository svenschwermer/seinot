#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define ERR(...) \
    do { \
        fprintf(stderr, "ERROR: " __VA_ARGS__); \
        fputc('\n', stderr); \
    } while (0)

#define INFO(...) \
    do { \
        fprintf(stderr, "INFO: " __VA_ARGS__); \
        fputc('\n', stderr); \
    } while (0)

#endif
