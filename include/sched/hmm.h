#ifndef SCHED_HMM_H
#define SCHED_HMM_H

#include "sched/structs.h"
#include <stdint.h>

typedef void(sched_hmm_set_func_t)(struct sched_hmm *, void *arg);

void sched_hmm_init(struct sched_hmm *);
enum sched_rc sched_hmm_set_file(struct sched_hmm *, char const *filename);

enum sched_rc sched_hmm_get_by_id(struct sched_hmm *, int64_t id);
enum sched_rc sched_hmm_get_by_job_id(struct sched_hmm *, int64_t job_id);
enum sched_rc sched_hmm_get_by_xxh3(struct sched_hmm *, int64_t xxh3);
enum sched_rc sched_hmm_get_by_filename(struct sched_hmm *,
                                        char const *filename);

enum sched_rc sched_hmm_get_all(sched_hmm_set_func_t, struct sched_hmm *,
                                void *arg);

enum sched_rc sched_hmm_remove(int64_t id);

#endif
