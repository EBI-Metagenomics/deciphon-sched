#ifndef SCHED_JOB_H
#define SCHED_JOB_H

#include "limits.h"
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
    char state[JOB_STATE_SIZE];

    char error[JOB_ERROR_SIZE];
    int64_t submission;
    int64_t exec_started;
    int64_t exec_ended;
};

struct sched_prod;
struct sched_seq;

typedef void(sched_seq_set_cb)(struct sched_seq *seq, void *arg);
typedef void(sched_prod_set_cb)(struct sched_prod *prod, void *arg);

void sched_job_init(struct sched_job *job, int64_t db_id, bool multi_hits,
                    bool hmmer3_compat);

enum sched_rc sched_job_get_seqs(int64_t job_id, sched_seq_set_cb cb,
                                 struct sched_seq *seq, void *arg);

enum sched_rc sched_job_get_prods(int64_t job_id, sched_prod_set_cb cb,
                                  struct sched_prod *prod, void *arg);

enum sched_rc sched_job_get(struct sched_job *job);

enum sched_rc sched_job_set_run(int64_t job_id);
enum sched_rc sched_job_set_fail(int64_t job_id, char const *msg);
enum sched_rc sched_job_set_done(int64_t job_id);

enum sched_rc sched_job_begin_submission(struct sched_job *job);
void sched_job_add_seq(struct sched_job *job, char const *name,
                       char const *data);
enum sched_rc sched_job_rollback_submission(struct sched_job *job);
enum sched_rc sched_job_end_submission(struct sched_job *job);

enum sched_rc sched_job_next_pend(struct sched_job *job);

#endif
