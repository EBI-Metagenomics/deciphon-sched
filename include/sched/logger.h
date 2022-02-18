#ifndef SCHED_LOGGER_H
#define SCHED_LOGGER_H

typedef void sched_logger_print_t(char const *msg, void *arg);

void sched_logger_setup(sched_logger_print_t *print, void *arg);

#endif
