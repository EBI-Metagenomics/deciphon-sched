#ifndef TO_H
#define TO_H

#include <stdbool.h>
#include <stdint.h>

bool to_double(char const *str, double *val);
bool to_int64(char const *str, int64_t *val);
bool to_int64l(unsigned len, char const *str, int64_t *val);
bool to_int(char const *str, int *val);
unsigned to_str(char *str, uint16_t num);

#endif
