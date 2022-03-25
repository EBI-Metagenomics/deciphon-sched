#ifndef XSTRCPY_H
#define XSTRCPY_H

#include "compiler.h"
#include "strlcpy.h"
#include <stdbool.h>
#include <stddef.h>

static inline bool xstrcpy(char *dst, char const *src, size_t dsize)
{
    return strlcpy(dst, src, dsize) < dsize;
}

#define XSTRCPY(ptr, member, src)                                              \
    xstrcpy((ptr)->member, src, ARRAY_SIZE_OF(*(ptr), member))

#endif
