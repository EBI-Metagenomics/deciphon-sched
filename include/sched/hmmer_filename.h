#ifndef SCHED_HMMER_FILENAME_H
#define SCHED_HMMER_FILENAME_H

#include "sched/structs.h"
#include <stdint.h>

void sched_hmmer_filename_setup(struct sched_hmmer_filename const *,
                                char *filename);
enum sched_rc sched_hmmer_filename_parse(struct sched_hmmer_filename *,
                                         char const *filename);

#endif
