#ifndef DCP_SCHED_JOB_H
#define DCP_SCHED_JOB_H

#include <stdbool.h>
#include <stdint.h>

enum sched_job_state
{
    JOB_PEND,
    JOB_RUN,
    JOB_DONE,
    JOB_FAIL
};

int sched_job_state(int64_t job_id, enum sched_job_state *state);

#endif
