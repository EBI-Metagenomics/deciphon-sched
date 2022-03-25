#ifndef XSTRCPY_H
#define XSTRCPY_H

#include "strlcpy.h"
#include <stdbool.h>
#include <stddef.h>

static inline bool xstrcpy(char *dst, char const *src, size_t dsize)
{
    return strlcpy(dst, src, dsize) < dsize;
}

#endif
