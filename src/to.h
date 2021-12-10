#ifndef TO_H
#define TO_H

#include <stdbool.h>
#include <stdint.h>

bool to_double(char const *str, double *val);
bool to_int64(char const *str, int64_t *val);
bool to_int(char const *str, int *val);

#endif
