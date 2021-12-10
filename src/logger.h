#ifndef LOGGER_H
#define LOGGER_H

#include "compiler.h"

typedef void logger_print_t(char const *msg, void *arg);
void logger_setup(logger_print_t *print, void *arg);

#define __ERROR_FMT(msg) LOCAL(__LINE__) ": " msg
int __logger_error(char const *msg);
#define error(msg) __logger_error(__ERROR_FMT(msg))

#endif
