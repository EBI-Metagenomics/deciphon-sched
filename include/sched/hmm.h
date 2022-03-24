#ifndef SCHED_HMM_H
#define SCHED_HMM_H

#include "sched/limits.h"
#include <stdint.h>

struct sched_hmm
{
    int64_t id;
    int64_t xxh3;
    char filename[FILENAME_SIZE];
    int64_t job_id;
};

typedef void(sched_hmm_set_func_t)(struct sched_hmm *, void *arg);

void sched_hmm_init(struct sched_hmm *);

enum sched_rc sched_hmm_get_by_id(struct sched_hmm *, int64_t id);
enum sched_rc sched_hmm_get_by_xxh3(struct sched_hmm *, int64_t xxh3);
enum sched_rc sched_hmm_get_by_filename(struct sched_hmm *,
                                        char const *filename);

#endif
