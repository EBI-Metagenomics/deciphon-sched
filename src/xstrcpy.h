#ifndef XSTRCPY_H
#define XSTRCPY_H

#include "compiler.h"
#include "logger.h"
#include "sched/rc.h"
#include "strlcpy.h"
#include <stdbool.h>
#include <stddef.h>

static inline enum sched_rc xstrcpy(char *dst, char const *src, size_t dsize)
{
    return strlcpy(dst, src, dsize) < dsize ? SCHED_OK
                                            : error(SCHED_NOT_ENOUGH_MEMORY);
}

#define XSTRCPY(ptr, member, src)                                              \
    xstrcpy((ptr)->member, src, ARRAY_SIZE_OF(*(ptr), member))

#endif
