#ifndef DCP_SCHED_SEQ_H
#define DCP_SCHED_SEQ_H

#include "dcp_sched/export.h"
#include "dcp_sched/limits.h"
#include <stdint.h>

struct sched_seq
{
    int64_t id;
    int64_t job_id;
    char name[SCHED_SEQ_NAME_SIZE];
    char data[SCHED_SEQ_SIZE];
};

SCHED_API void sched_seq_init(struct sched_seq *seq, int64_t job_id,
                              char const *name, char const *data);
SCHED_API int sched_seq_next(struct sched_seq *seq);

#endif
