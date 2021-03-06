#ifndef JOB_H
#define JOB_H

#include "sched/job.h"
#include <stdbool.h>
#include <stdint.h>

enum sched_rc job_set_run(int64_t job_id, int64_t exec_started);
enum sched_rc job_set_error(int64_t job_id, char const *error,
                            int64_t exec_ended);
enum sched_rc job_set_done(int64_t job_id, int64_t exec_ended);
enum sched_rc job_wipe(void);

#endif
