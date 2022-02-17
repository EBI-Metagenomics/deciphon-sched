#include "to.h"
#include <limits.h>
#include <stdlib.h>
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

bool to_int64l(unsigned len, char const *str, int64_t *val)
{
    char *s = (char *)str;
    char c = str[len];
    s[len] = '\0';
    bool ret = to_int64(str, val);
    s[len] = c;
    return ret;
}

bool to_int(char const *str, int *val)
{
    long v = strtol(str, NULL, 10);
    *val = (int)v;
    return (v <= INT_MAX || !(v == 0 && !(str[0] == '0' && str[1] == '\0')));
}

/* Convert uint16_t to null-terminated string.
 * Return the string length. */
unsigned to_str(char *str, uint16_t num)
{
    char const *const begin = str;
    *str = '0';

    unsigned denom = 10000;
    while (denom > 1 && !(num / denom))
        denom /= 10;

    while (denom)
    {
        *str = (char)('0' + (num / denom));
        num %= denom;
        ++str;
        denom /= 10;
    }
    *str = 0;
    return (unsigned)(str - begin);
}
