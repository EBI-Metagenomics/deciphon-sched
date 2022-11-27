#ifndef SCHED_HMMER_H
#define SCHED_HMMER_H

#include "sched/structs.h"
#include <stdint.h>

void sched_hmmer_init(struct sched_hmmer *, int64_t prod_id);

enum sched_rc sched_hmmer_get_by_id(struct sched_hmmer *, int64_t id);
enum sched_rc sched_hmmer_get_by_prod_id(struct sched_hmmer *, int64_t prod_id);

enum sched_rc sched_hmmer_add(struct sched_hmmer *, int len,
                              unsigned char const *data);

enum sched_rc sched_hmmer_remove(int64_t id);

#endif
