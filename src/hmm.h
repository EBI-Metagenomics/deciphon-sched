#ifndef HMM_H
#define HMM_H

#include "sched/hmm.h"
#include "sched/limits.h"
#include <stdint.h>

// enum sched_rc hmm_has(char const *filename, struct sched_hmm *);
// enum sched_rc hmm_hash(char const *filename, int64_t *xxh64);
enum sched_rc hmm_get_by_id(struct sched_hmm *, int64_t id);
enum sched_rc hmm_get_by_xxh3(struct sched_hmm *, int64_t xxh3);
enum sched_rc hmm_delete(void);
enum sched_rc hmm_submit(void *hmm, int64_t job_id);

#endif
