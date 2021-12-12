#ifndef JOB_H
#define JOB_H

#include "dcp_sched/job.h"
#include "dcp_sched/limits.h"
#include <stdbool.h>
#include <stdint.h>

int job_submit(struct sched_job *job);
int job_next_pending(struct sched_job *job);
int job_set_error(int64_t job_id, char const *error, int64_t exec_ended);
int job_set_done(int64_t job_id, int64_t exec_ended);
int job_get(struct sched_job *job);

int job_module_init(void);
void job_module_del(void);

#endif
