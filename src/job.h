#ifndef JOB_H
#define JOB_H

#include "dcp_sched/job.h"
#include "dcp_sched/limits.h"
#include <stdint.h>

struct job
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

void job_init(int64_t db_id, bool multi_hits, bool hmmer3_compat);
int job_submit(void);
int job_next_pending(void);
int job_set_error(int64_t job_id, char const *error, int64_t exec_ended);
int job_set_done(int64_t job_id, int64_t exec_ended);
int job_get(int64_t job_id);

int job_module_init(void);
void job_module_del(void);

#endif
