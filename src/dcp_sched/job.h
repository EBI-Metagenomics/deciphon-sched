#ifndef DCP_SCHED_JOB_H
#define DCP_SCHED_JOB_H

#include "dcp_sched/export.h"
#include "dcp_sched/limits.h"
#include <stdbool.h>
#include <stdint.h>

enum sched_job_state
{
    SCHED_JOB_PEND,
    SCHED_JOB_RUN,
    SCHED_JOB_DONE,
    SCHED_JOB_FAIL
};

struct sched_job
{
    int64_t id;

    int64_t db_id;
    int32_t multi_hits;
    int32_t hmmer3_compat;
    char state[SCHED_JOB_STATE_SIZE];

    char error[SCHED_JOB_ERROR_SIZE];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

SCHED_API void sched_job_init(struct sched_job *job, int64_t db_id,
                              bool multi_hits, bool hmmer3_compat);

SCHED_API int sched_job_state(int64_t job_id, enum sched_job_state *state);

#endif
