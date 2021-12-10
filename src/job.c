#include "job.h"
#include "compiler.h"
#include "dcp_sched/job.h"
#include "dcp_sched/rc.h"
#include "logger.h"
#include "safe.h"
#include "sched.h"
#include "sched_limits.h"
#include "seq.h"
#include "utc.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

struct job job = {0};
extern struct sqlite3 *sched;

enum
{
    INSERT,
    GET_PEND,
    GET_STATE,
    SELECT,
    SET_ERROR,
    SET_DONE
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] =\
"\
        INSERT INTO job\
            (\
                db_id, multi_hits, hmmer3_compat,      state,\
                error, submission, exec_started,  exec_ended\
            )\
        VALUES\
            (\
                ?, ?, ?, ?,\
                ?, ?, ?, ?\
            )\
        RETURNING id;\
",
    [GET_PEND] = \
"\
        UPDATE job SET\
            state = 'run', exec_started = ?\
        WHERE \
            id = (SELECT MIN(id) FROM job WHERE state = 'pend' LIMIT 1)\
        RETURNING id;\
",
    [GET_STATE] = "SELECT state FROM job WHERE id = ?;",
    [SELECT] = "SELECT * FROM job WHERE id = ?;\
",
    [SET_ERROR] = \
"\
    UPDATE job SET\
        state = 'fail', error = ?, exec_ended = ? WHERE id = ?;\
",
    [SET_DONE] = \
"\
    UPDATE job SET\
        state = 'done', exec_ended = ? WHERE id = ?;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

int job_module_init(void)
{
    int rc = 0;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if ((rc = xsql_prepare(sched, queries[i], stmts + i))) return rc;
    }
    return 0;
}

void job_init(int64_t db_id, bool multi_hits, bool hmmer3_compat)
{
    job.id = 0;
    job.db_id = db_id;
    job.multi_hits = multi_hits;
    job.hmmer3_compat = hmmer3_compat;
}

int job_submit(void)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, job.db_id))) return rc;
    if ((rc = xsql_bind_i64(stmt, 1, job.multi_hits))) return rc;
    if ((rc = xsql_bind_i64(stmt, 2, job.hmmer3_compat))) return rc;
    if ((rc = xsql_bind_str(stmt, 3, job.state))) return rc;

    if ((rc = xsql_bind_str(stmt, 4, job.error))) return rc;
    if ((rc = xsql_bind_i64(stmt, 5, job.submission))) return rc;
    if ((rc = xsql_bind_i64(stmt, 6, 0))) return rc;
    if ((rc = xsql_bind_i64(stmt, 7, 0))) return rc;

    rc = xsql_step(stmt);
    if (rc != 2) return rc;
    job.id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

static int next_pending_job_id(int64_t *job_id)
{
    struct sqlite3_stmt *stmt = stmts[GET_PEND];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, (int64_t)utc_now()))) return rc;

    rc = xsql_step(stmt);
    if (rc == 0) return DCP_SCHED_NOTFOUND;
    if (rc != 2) return DCP_SCHED_FAIL;

    *job_id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

int job_next_pending(void)
{
    int rc = next_pending_job_id(&job.id);
    if (rc) return rc;
    return job_get(job.id);
}

int job_set_error(int64_t job_id, char const *error, int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_ERROR];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_str(stmt, 0, error))) return rc;
    if ((rc = xsql_bind_i64(stmt, 1, exec_ended))) return rc;
    if ((rc = xsql_bind_i64(stmt, 2, job_id))) return rc;

    return xsql_end_step(stmt);
}

int job_set_done(int64_t job_id, int64_t exec_ended)
{
    struct sqlite3_stmt *stmt = stmts[SET_DONE];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, exec_ended))) return rc;
    if ((rc = xsql_bind_i64(stmt, 1, job_id))) return rc;

    return xsql_end_step(stmt);
}

static enum dcp_sched_job_state resolve_job_state(char const *state)
{
    if (strcmp("pend", state) == 0)
        return JOB_PEND;
    else if (strcmp("run", state) == 0)
        return JOB_RUN;
    else if (strcmp("done", state) == 0)
        return JOB_DONE;
    else if (strcmp("fail", state) == 0)
        return JOB_FAIL;

    exit(EXIT_FAILURE);
    return 0;
}

int dcp_sched_job_state(int64_t job_id, enum dcp_sched_job_state *state)
{
    struct sqlite3_stmt *stmt = stmts[GET_STATE];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, job_id))) return rc;

    rc = xsql_step(stmt);
    if (rc == 0) return DCP_SCHED_NOTFOUND;
    if (rc != 2) return DCP_SCHED_FAIL;

    char tmp[SCHED_JOB_STATE_SIZE] = {0};
    rc = xsql_cpy_txt(stmt, 0, (struct xsql_txt){SCHED_JOB_STATE_SIZE, tmp});
    if (rc) return rc;
    *state = resolve_job_state(tmp);

    return xsql_end_step(stmt);
}

int job_get(int64_t job_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, job_id))) return rc;

    rc = xsql_step(stmt);
    if (rc != 2) return error("failed to get job");

    job.id = sqlite3_column_int64(stmt, 0);

    job.db_id = sqlite3_column_int64(stmt, 1);
    job.multi_hits = sqlite3_column_int(stmt, 2);
    job.hmmer3_compat = sqlite3_column_int(stmt, 3);
    rc = xsql_cpy_txt(stmt, 4, XSQL_TXT_OF(job, state));
    if (rc) return rc;

    rc = xsql_cpy_txt(stmt, 5, XSQL_TXT_OF(job, error));
    if (rc) return rc;
    job.submission = sqlite3_column_int64(stmt, 6);
    job.exec_started = sqlite3_column_int64(stmt, 7);
    job.exec_ended = sqlite3_column_int64(stmt, 8);

    return xsql_end_step(stmt);
}

void job_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
