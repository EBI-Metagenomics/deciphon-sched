#ifndef UTC_H
#define UTC_H

#include <stdint.h>
#include <time.h>

static inline uint64_t utc_now(void) { return (uint64_t)time(NULL); }

#endif
