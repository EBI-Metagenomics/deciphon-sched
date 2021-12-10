#ifndef DCP_SCHED_JOB_H
#define DCP_SCHED_JOB_H

#include <stdbool.h>
#include <stdint.h>

enum dcp_sched_job_state
{
    JOB_PEND,
    JOB_RUN,
    JOB_DONE,
    JOB_FAIL
};

int dcp_sched_job_state(int64_t job_id, enum dcp_sched_job_state *state);

#endif
