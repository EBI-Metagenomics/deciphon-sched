#ifndef LOGGER_H
#define LOGGER_H

#include "compiler.h"
#include "sched/logger.h"
#include "sched/rc.h"

#define error(rc, msg) __logger_error(rc, LOCAL, msg)

#define eio(msg) error(SCHED_EIO, msg)
#define efail(msg) error(SCHED_EFAIL, msg)
#define einval(msg) error(SCHED_EINVAL, msg)
#define enomem(msg) error(SCHED_ENOMEM, msg)
#define eparse(msg) error(SCHED_EPARSE, msg)
#define econstraint(msg) error(SCHED_ECONSTRAINT, msg)

enum sched_rc __logger_error(enum sched_rc rc, char const *ctx,
                             char const *msg);

#define EBIND efail("bind")
#define ECPYTXT efail("copy txt")
#define ESTEP efail("step")
#define EFRESH efail("get fresh statement")
#define EOPENSCHED efail("failed to open sched")
#define EEXEC efail("failed to exec")

#endif
