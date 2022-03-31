#ifndef SCHED_JOB_H
#define SCHED_JOB_H

#include "sched/limits.h"
#include <stdbool.h>
#include <stdint.h>

enum sched_job_type
{
    SCHED_SCAN,
    SCHED_HMM
};

enum sched_job_state
{
    SCHED_PEND,
    SCHED_RUN,
    SCHED_DONE,
    SCHED_FAIL
};

struct sched_job
{
    int64_t id;
    int type;

    char state[JOB_STATE_SIZE];
    int progress;
    char error[JOB_ERROR_SIZE];

    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

typedef void(sched_job_set_func_t)(struct sched_job *, void *arg);

void sched_job_init(struct sched_job *, enum sched_job_type);

enum sched_rc sched_job_get_by_id(struct sched_job *, int64_t id);
enum sched_rc sched_job_get_all(sched_job_set_func_t, struct sched_job *,
                                void *arg);
enum sched_rc sched_job_next_pend(struct sched_job *);

enum sched_rc sched_job_set_run(int64_t id);
enum sched_rc sched_job_set_fail(int64_t id, char const *msg);
enum sched_rc sched_job_set_done(int64_t id);

enum sched_rc sched_job_submit(struct sched_job *, void *actual_job);

enum sched_rc sched_job_add_progress(int64_t id, int progress);

enum sched_rc sched_job_remove(int64_t id);

#endif
