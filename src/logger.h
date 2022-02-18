#ifndef LOGGER_H
#define LOGGER_H

#include "compiler.h"
#include "sched/rc.h"

typedef void logger_print_t(char const *msg, void *arg);
void logger_setup(logger_print_t *print, void *arg);

#define __WARN_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum sched_rc __logger_warn(enum sched_rc rc, char const *msg);
#define warn(rc, msg) __logger_warn(rc, __WARN_FMT(rc, msg))

#define __ERROR_FMT(rc, msg) LOCAL(__LINE__) ":" #rc ": " msg
enum sched_rc __logger_error(enum sched_rc rc, char const *msg);
#define error(rc, msg) __logger_error(rc, __ERROR_FMT(rc, msg))

#define eio(what) error(SCHED_EIO, "failed to " what)
#define efail(what) error(SCHED_EFAIL, "failed to " what)
#define einval(what) error(SCHED_EINVAL, "failed to" what)
#define eparse(what) error(SCHED_EPARSE, "failed to " what)

#endif
