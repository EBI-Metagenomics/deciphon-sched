#ifndef BUG_H
#define BUG_H

#include "compiler.h"

/*
 * Acknowledgement: Linux kernel developers.
 */
#define BUG()                                                                  \
    do                                                                         \
    {                                                                          \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        exit(EXIT_FAILURE);                                                    \
        __builtin_unreachable();                                               \
    } while (1)

#endif
