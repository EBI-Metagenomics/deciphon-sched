#include "logger.h"
#include <stdio.h>

static void default_print(char const *msg, void *arg)
{
    unused(arg);
    fprintf(stderr, "%s\n", msg);
}

static sched_logger_print_t *__log_print = default_print;
static void *__log_arg = NULL;

void sched_logger_setup(sched_logger_print_t *print, void *arg)
{
    __log_print = print;
    __log_arg = arg;
}

static void log_print(char const *msg) { __log_print(msg, __log_arg); }

enum sched_rc __logger_error(enum sched_rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}

enum sched_rc __logger_warn(enum sched_rc rc, char const *msg)
{
    log_print(msg);
    return rc;
}
