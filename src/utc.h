#ifndef UTC_H
#define UTC_H

#include <stdint.h>
#include <time.h>

static inline int64_t utc_now(void) { return (int64_t)time(NULL); }

#endif
