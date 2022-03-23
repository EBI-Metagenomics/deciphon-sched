#include "job.h"
#include "bug.h"
#include "compiler.h"
#include "logger.h"
#include "scan.h"
#include "sched/job.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "stmt.h"
#include "strlcpy.h"
#include "utc.h"
#include "xsql.h"
#include <stdlib.h>
#include <string.h>

typedef enum sched_rc (*submit_job_func_t)(void *actual_job, int64_t job_id);

static submit_job_func_t submit_job_func[] = {[SCHED_SCAN] = scan_submit};

void sched_job_init(struct sched_job *job, enum sched_job_type type)
{
    job->id = 0;
    job->type = (int)type;

    strlcpy(job->state, "pend", ARRAY_SIZE_OF(*job, state));
    job->progress = 0;
    job->error[0] = 0;

    job->submission = 0;
    job->exec_started = 0;
    job->exec_ended = 0;
}

enum sched_rc sched_job_get(struct sched_job *job)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_SELECT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, job->id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get job");

    job->id = xsql_get_i64(st, 0);
    job->type = xsql_get_int(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*job, state))) efail("copy txt");
    job->progress = xsql_get_int(st, 3);
    if (xsql_cpy_txt(st, 4, XSQL_TXT_OF(*job, error))) efail("copy txt");

    job->submission = xsql_get_i64(st, 6);
    job->exec_started = xsql_get_i64(st, 7);
    job->exec_ended = xsql_get_i64(st, 8);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

static enum sched_rc next_pend_job_id(int64_t *id)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_GET_PEND);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

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
    return sched_job_get(job);
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
    struct sqlite3 *sched = sched_handle();
    if (xsql_begin_transaction(sched)) return efail("begin job submission");
    return SCHED_OK;
}

static enum sched_rc rollback_submission(void)
{
    struct sqlite3 *sched = sched_handle();
    return xsql_rollback_transaction(sched);
}

static enum sched_rc submit_job(struct sched_job *job)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_INSERT);
    job->submission = utc_now();
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, job->type)) return efail("bind");

    if (xsql_bind_str(st, 1, job->state)) return efail("bind");
    if (xsql_bind_i64(st, 2, job->progress)) return efail("bind");
    if (xsql_bind_str(st, 3, job->error)) return efail("bind");

    if (xsql_bind_i64(st, 4, job->submission)) return efail("bind");
    if (xsql_bind_i64(st, 5, job->exec_started)) return efail("bind");
    if (xsql_bind_i64(st, 6, job->exec_ended)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    job->id = xsql_last_id(sched);
    return SCHED_OK;
}

static enum sched_rc end_submission(void)
{
    struct sqlite3 *sched = sched_handle();
    return xsql_end_transaction(sched);
}

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

enum sched_rc job_set_run(int64_t id, int64_t exec_started)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_SET_RUN);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, exec_started)) return efail("bind");
    if (xsql_bind_i64(st, 1, id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc job_set_error(int64_t id, char const *error, int64_t exec_ended)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_SET_ERROR);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_str(st, 0, error)) return efail("bind");
    if (xsql_bind_i64(st, 1, exec_ended)) return efail("bind");
    if (xsql_bind_i64(st, 2, id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc job_set_done(int64_t id, int64_t exec_ended)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_SET_DONE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, exec_ended)) return efail("bind");
    if (xsql_bind_i64(st, 1, id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc job_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete db");
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

enum sched_rc sched_job_state(int64_t id, enum sched_job_state *state)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(JOB_GET_STATE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, id)) return efail("bind");

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
