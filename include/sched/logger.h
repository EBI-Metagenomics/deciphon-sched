#ifndef SCHED_LOGGER_H
#define SCHED_LOGGER_H

/* It assumes that `msg` is of static storage class. */
typedef void (*sched_logger_print_func_t)(char const *ctx, char const *msg,
                                          void *arg);

void sched_logger_setup(sched_logger_print_func_t, void *arg);

#endif
