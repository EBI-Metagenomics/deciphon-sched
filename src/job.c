#include "job.h"
#include "bug.h"
#include "hmm.h"
#include "logger.h"
#include "scan.h"
#include "sched/job.h"
#include "sched/rc.h"
#include "stmt.h"
#include "utc.h"
#include "xsql.h"
#include "xstrcpy.h"
#include <stdlib.h>
#include <string.h>

typedef enum sched_rc (*submit_job_func_t)(void *actual_job, int64_t job_id);

static submit_job_func_t submit_job_func[] = {
    [SCHED_SCAN] = scan_submit, [SCHED_HMM] = hmm_submit};

static void job_init(struct sched_job *job)
{
    job->id = 0;
    job->type = 0;

    XSTRCPY(job, state, "pend");
    job->progress = 0;
    job->error[0] = 0;

    job->submission = 0;
    job->exec_started = 0;
    job->exec_ended = 0;
}

void sched_job_init(struct sched_job *job, enum sched_job_type type)
{
    job_init(job);
    job->type = (int)type;
}

static enum sched_rc set_job(struct sched_job *job, struct sqlite3_stmt *st)
{
    job->id = xsql_get_i64(st, 0);
    job->type = xsql_get_int(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*job, state))) ECPYTXT;
    job->progress = xsql_get_int(st, 3);
    if (xsql_cpy_txt(st, 4, XSQL_TXT_OF(*job, error))) ECPYTXT;

    job->submission = xsql_get_i64(st, 5);
    job->exec_started = xsql_get_i64(st, 6);
    job->exec_ended = xsql_get_i64(st, 7);

    return SCHED_OK;
}

enum sched_rc sched_job_get_by_id(struct sched_job *job, int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_GET));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get job");

    if ((rc = set_job(job, st))) return rc;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

static enum sched_rc job_next(struct sched_job *job)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_GET_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, job->id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return ESTEP;

    if ((rc = set_job(job, st))) return rc;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_job_get_all(sched_job_set_func_t fn, struct sched_job *job,
                                void *arg)
{
    enum sched_rc rc = SCHED_OK;

    job_init(job);
    while ((rc = job_next(job)) == SCHED_OK)
        fn(job, arg);
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

static enum sched_rc next_pend_job_id(int64_t *id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_GET_PEND));
    if (!st) return EFRESH;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get pend job");
    *id = xsql_get_i64(st, 0);

    if (xsql_step(st) != SCHED_END) return efail("get pend job");
    return SCHED_OK;
}

enum sched_rc sched_job_next_pend(struct sched_job *job)
{
    enum sched_rc rc = next_pend_job_id(&job->id);
    if (rc == SCHED_NOTFOUND) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get next pend job");
    return sched_job_get_by_id(job, job->id);
}

enum sched_rc sched_job_set_run(int64_t id)
{
    return job_set_run(id, utc_now());
}

enum sched_rc sched_job_set_fail(int64_t id, char const *msg)
{
    return job_set_error(id, msg, utc_now());
}
enum sched_rc sched_job_set_done(int64_t id)
{
    return job_set_done(id, utc_now());
}

static enum sched_rc begin_submission(void)
{
    if (xsql_begin_transaction()) return efail("begin job submission");
    return SCHED_OK;
}

static enum sched_rc rollback_submission(void)
{
    return xsql_rollback_transaction();
}

static enum sched_rc submit_job(struct sched_job *job)
{
    job->submission = utc_now();
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_INSERT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, job->type)) return EBIND;

    if (xsql_bind_str(st, 1, job->state)) return EBIND;
    if (xsql_bind_i64(st, 2, job->progress)) return EBIND;
    if (xsql_bind_str(st, 3, job->error)) return EBIND;

    if (xsql_bind_i64(st, 4, job->submission)) return EBIND;
    if (xsql_bind_i64(st, 5, job->exec_started)) return EBIND;
    if (xsql_bind_i64(st, 6, job->exec_ended)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    job->id = xsql_last_id();
    return SCHED_OK;
}

static enum sched_rc end_submission(void) { return xsql_end_transaction(); }

enum sched_rc sched_job_submit(struct sched_job *job, void *actual_job)
{
    enum sched_rc rc = begin_submission();
    if (rc) return rc;

    if ((rc = submit_job(job))) goto cleanup;
    if ((rc = submit_job_func[job->type](actual_job, job->id))) goto cleanup;

    return end_submission();

cleanup:
    rollback_submission();
    return rc;
}

enum sched_rc sched_job_add_progress(int64_t id, int progress)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_ADD_PROGRESS));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, progress)) return EBIND;
    if (xsql_bind_i64(st, 1, id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc sched_job_remove(int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_DELETE_BY_ID));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc job_set_run(int64_t id, int64_t exec_started)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_SET_RUN));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, exec_started)) return EBIND;
    if (xsql_bind_i64(st, 1, id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc job_set_error(int64_t id, char const *error, int64_t exec_ended)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_SET_ERROR));
    if (!st) return EFRESH;

    if (xsql_bind_str(st, 0, error)) return EBIND;
    if (xsql_bind_i64(st, 1, exec_ended)) return EBIND;
    if (xsql_bind_i64(st, 2, id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc job_set_done(int64_t id, int64_t exec_ended)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_SET_DONE));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, exec_ended)) return EBIND;
    if (xsql_bind_i64(st, 1, id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc job_delete(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_DELETE));
    if (!st) return EFRESH;

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete job");
}

static enum sched_job_state resolve_job_state(char const *state)
{
    if (strcmp("pend", state) == 0)
        return SCHED_PEND;
    else if (strcmp("run", state) == 0)
        return SCHED_RUN;
    else if (strcmp("done", state) == 0)
        return SCHED_DONE;
    else if (strcmp("fail", state) == 0)
        return SCHED_FAIL;

    BUG();
}

enum sched_rc sched_job_state(int64_t id, enum sched_job_state *state)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(JOB_GET_STATE));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get job state");

    char job_state[JOB_STATE_SIZE] = {0};
    rc = xsql_cpy_txt(st, 0, (struct xsql_txt){JOB_STATE_SIZE, job_state});
    if (rc) ECPYTXT;
    *state = resolve_job_state(job_state);

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}
