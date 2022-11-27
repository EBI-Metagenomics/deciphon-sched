#include "ltoa.h"

static void reverse(signed char buf[], int size)
{
    signed char *end = buf + size - 1;

    while (buf < end)
    {
        signed char tmp = *buf;
        *buf = *end;
        *end = tmp;
        buf++;
        end--;
    }
}

static int ltoa_rev(char *buf, long i);

int sched_ltoa(char *buf, long i)
{
    int n = ltoa_rev(buf, i);
    reverse((signed char *)buf, n);
    return n;
}

// ACK: nanoprintf
static int ltoa_rev(char *buf, long i)
{
    int n = 0;
    long const sign = (i >= 0) ? 1 : -1;
    do
    {
        *buf++ = (char)('0' + (sign * (i % 10)));
        i /= 10;
        ++n;
    } while (i);
    return n;
}
