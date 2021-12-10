#include "to.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

bool to_double(char const *str, double *val)
{
    char *ptr = NULL;
    *val = strtod(str, &ptr);
    return !((*val == 0.0 && str == ptr) || strchr(str, '\0') != ptr);
}

bool to_int64(char const *str, int64_t *val)
{
    long long v = strtoll(str, NULL, 10);
    *val = (int64_t)v;
    return (v <= INT64_MAX || !(v == 0 && !(str[0] == '0' && str[1] == '\0')));
}

bool to_int(char const *str, int *val)
{
    long v = strtol(str, NULL, 10);
    *val = (int)v;
    return (v <= INT_MAX || !(v == 0 && !(str[0] == '0' && str[1] == '\0')));
}
