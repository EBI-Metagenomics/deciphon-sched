#include "job.h"
#include "bug.h"
#include "compiler.h"
#include "logger.h"
#include "prod.h"
#include "sched/job.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "seq.h"
#include "seq_queue.h"
#include "sqlite3/sqlite3.h"
#include "stmt.h"
#include "strlcpy.h"
#include "utc.h"
#include "xsql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct sqlite3 *sched;

void sched_job_init(struct sched_job *job, int64_t db_id, bool multi_hits,
                    bool hmmer3_compat)
{
    job->id = 0;
    job->db_id = db_id;
    job->multi_hits = multi_hits;
    job->hmmer3_compat = hmmer3_compat;
    strlcpy(job->state, "pend", ARRAY_SIZE_OF(*job, state));

    job->error[0] = 0;
    job->submission = 0;
    job->exec_started = 0;
    job->exec_ended = 0;
}

enum sched_rc sched_job_set_run(int64_t job_id)
{
    return job_set_run(job_id, utc_now());
}

enum sched_rc sched_job_set_fail(int64_t job_id, char const *msg)
{
    return job_set_error(job_id, msg, utc_now());
}
enum sched_rc sched_job_set_done(int64_t job_id)
{
    return job_set_done(job_id, utc_now());
}

enum sched_rc job_submit(struct sched_job *job)
{
    job->submission = utc_now();
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_INSERT]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, job->db_id)) return efail("bind");
    if (xsql_bind_i64(st, 1, job->multi_hits)) return efail("bind");
    if (xsql_bind_i64(st, 2, job->hmmer3_compat)) return efail("bind");
    if (xsql_bind_str(st, 3, job->state)) return efail("bind");

    if (xsql_bind_str(st, 4, job->error)) return efail("bind");
    if (xsql_bind_i64(st, 5, job->submission)) return efail("bind");
    if (xsql_bind_i64(st, 6, job->exec_started)) return efail("bind");
    if (xsql_bind_i64(st, 7, job->exec_ended)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    job->id = xsql_last_id(sched);
    return SCHED_OK;
}

static enum sched_rc next_pend_job_id(int64_t *job_id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_GET_PEND]);
    if (!st) return efail("get fresh statement");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get pend job");
    *job_id = sqlite3_column_int64(st, 0);

    if (xsql_step(st) != SCHED_END) return efail("get pend job");
    return SCHED_OK;
}

static enum sched_rc next_pending_job_id(int64_t *job_id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_GET_PEND]);
    if (!st) return efail("get fresh statement");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get pend job");
    *job_id = sqlite3_column_int64(st, 0);
    if (xsql_step(st) != SCHED_END) return efail("get pend job");

    st = xsql_fresh_stmt(sched, &stmt[JOB_SET_RUN]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, utc_now())) return efail("bind");
    if (xsql_bind_i64(st, 1, *job_id)) return efail("bind");
    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc job_next_pend(struct sched_job *job)
{
    enum sched_rc rc = next_pend_job_id(&job->id);
    if (rc == SCHED_NOTFOUND) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get next pend job");
    return sched_job_get(job);
}

enum sched_rc job_next_pending(struct sched_job *job)
{
    enum sched_rc rc = next_pending_job_id(&job->id);
    if (rc == SCHED_NOTFOUND) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get next pending job");
    return sched_job_get(job);
}

enum sched_rc job_set_run(int64_t job_id, int64_t exec_started)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_SET_RUN]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, exec_started)) return efail("bind");
    if (xsql_bind_i64(st, 1, job_id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc job_set_error(int64_t job_id, char const *error,
                            int64_t exec_ended)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_SET_ERROR]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_str(st, 0, error)) return efail("bind");
    if (xsql_bind_i64(st, 1, exec_ended)) return efail("bind");
    if (xsql_bind_i64(st, 2, job_id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc job_set_done(int64_t job_id, int64_t exec_ended)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_SET_DONE]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, exec_ended)) return efail("bind");
    if (xsql_bind_i64(st, 1, job_id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

static enum sched_job_state resolve_job_state(char const *state)
{
    if (strcmp("pend", state) == 0)
        return SCHED_JOB_PEND;
    else if (strcmp("run", state) == 0)
        return SCHED_JOB_RUN;
    else if (strcmp("done", state) == 0)
        return SCHED_JOB_DONE;
    else if (strcmp("fail", state) == 0)
        return SCHED_JOB_FAIL;

    BUG();
}

enum sched_rc sched_job_state(int64_t job_id, enum sched_job_state *state)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_GET_STATE]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, job_id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get job state");

    char tmp[JOB_STATE_SIZE] = {0};
    rc = xsql_cpy_txt(st, 0, (struct xsql_txt){JOB_STATE_SIZE, tmp});
    if (rc) efail("copy txt");
    *state = resolve_job_state(tmp);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc sched_job_get(struct sched_job *job)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, &stmt[JOB_SELECT]);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, job->id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get job");

    job->id = sqlite3_column_int64(st, 0);

    job->db_id = sqlite3_column_int64(st, 1);
    job->multi_hits = sqlite3_column_int(st, 2);
    job->hmmer3_compat = sqlite3_column_int(st, 3);
    if (xsql_cpy_txt(st, 4, XSQL_TXT_OF(*job, state))) efail("copy txt");

    if (xsql_cpy_txt(st, 5, XSQL_TXT_OF(*job, error))) efail("copy txt");
    job->submission = sqlite3_column_int64(st, 6);
    job->exec_started = sqlite3_column_int64(st, 7);
    job->exec_ended = sqlite3_column_int64(st, 8);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc sched_job_get_prods(int64_t job_id, sched_prod_set_cb cb,
                                  struct sched_prod *prod, void *arg)
{
    struct sched_job job = {0};
    job.id = job_id;
    enum sched_rc rc = sched_job_get(&job);
    if (rc) return rc;

    sched_prod_init(prod, job_id);
    while ((rc = prod_next(prod)) == SCHED_OK)
    {
        cb(prod, arg);
    }
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

enum sched_rc sched_job_get_seqs(int64_t job_id, sched_seq_set_cb cb,
                                 struct sched_seq *seq, void *arg)
{
    struct sched_job job = {0};
    job.id = job_id;
    enum sched_rc rc = sched_job_get(&job);
    if (rc) return rc;

    seq->id = 0;
    seq->job_id = job_id;
    while ((rc = sched_seq_next(seq)) == SCHED_OK)
    {
        cb(seq, arg);
    }
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

enum sched_rc sched_job_begin_submission(struct sched_job *job)
{
    sched_job_init(job, job->db_id, job->multi_hits, job->hmmer3_compat);
    if (xsql_begin_transaction(sched)) return efail("begin job submission");
    seq_queue_init();
    return SCHED_OK;
}

void sched_job_add_seq(struct sched_job *job, char const *name,
                       char const *data)
{
    seq_queue_add(job->id, name, data);
}

enum sched_rc sched_job_rollback_submission(struct sched_job *job)
{
    return xsql_rollback_transaction(sched);
}

enum sched_rc sched_job_end_submission(struct sched_job *job)
{
    if (job_submit(job)) return efail("end job submission");

    for (unsigned i = 0; i < seq_queue_size(); ++i)
    {
        struct sched_seq *seq = seq_queue_get(i);
        seq->job_id = job->id;
        if (seq_submit(seq)) return xsql_rollback_transaction(sched);
    }

    return xsql_end_transaction(sched);
}
