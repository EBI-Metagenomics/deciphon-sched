#include "logger.h"
#include <stdio.h>

static void default_print(char const *ctx, char const *msg, void *arg)
{
    fputs(ctx, stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    unused(arg);
}

static struct
{
    sched_logger_print_func_t print;
    void *arg;
} local = {default_print, 0};

void sched_logger_setup(sched_logger_print_func_t print, void *arg)
{
    local.print = print;
    local.arg = arg;
}

enum sched_rc __logger_error(enum sched_rc rc, char const *ctx, char const *msg)
{
    local.print(ctx, msg, local.arg);
    return rc;
}
