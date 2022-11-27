#include "stmt.h"
#include "compiler.h"
#include "error.h"
#include "sched.h"
#include "sched/sched.h"
#include "xsql.h"
#include <assert.h>

/* clang-format off */
static char const *const queries[] =
{
    /* --- HMM queries --- */
    [HMM_INSERT] = "INSERT INTO hmm (xxh3, filename, job_id) VALUES (?, ?, ?);",

    [HMM_GET_BY_ID]       = "SELECT * FROM hmm WHERE       id = ?;",
    [HMM_GET_BY_JOB_ID]   = "SELECT * FROM hmm WHERE   job_id = ?;",
    [HMM_GET_BY_XXH3]     = "SELECT * FROM hmm WHERE    xxh3  = ?;",
    [HMM_GET_BY_FILENAME] = "SELECT * FROM hmm WHERE filename = ?;",
    [HMM_GET_NEXT]        = "SELECT * FROM hmm WHERE id > ? ORDER BY id ASC LIMIT 1;",

    [HMM_DELETE_BY_ID]    = "DELETE FROM hmm WHERE id = ?;",
    [HMM_DELETE]          = "DELETE FROM hmm;",

    /* --- DB queries --- */
    [DB_INSERT] = "INSERT INTO db (xxh3, filename, hmm_id) VALUES (?, ?, ?);",

    [DB_GET_BY_ID]       = "SELECT * FROM db WHERE       id = ?;",
    [DB_GET_BY_XXH3]     = "SELECT * FROM db WHERE     xxh3 = ?;",
    [DB_GET_BY_FILENAME] = "SELECT * FROM db WHERE filename = ?;",
    [DB_GET_BY_HMM_ID]   = "SELECT * FROM db WHERE   hmm_id = ?;",
    [DB_GET_NEXT]        = "SELECT * FROM db WHERE id > ? ORDER BY id ASC LIMIT 1;",

    [DB_DELETE_BY_ID] = "DELETE FROM db WHERE id = ?;",
    [DB_DELETE]       = "DELETE FROM db;",

    /* --- JOB queries --- */
    [JOB_INSERT] = "INSERT INTO job (type, state, progress, error, submission, exec_started, exec_ended) "
                   "VALUES          (   ?,     ?,        ?,     ?,          ?,            ?,          ?);",

    [JOB_GET_PEND]  = "SELECT    id FROM job WHERE state = 'pend' ORDER BY id LIMIT 1;",
    [JOB_GET_STATE] = "SELECT state FROM job WHERE    id = ?;",
    [JOB_GET]       = "SELECT     * FROM job WHERE    id = ?;",
    [JOB_GET_NEXT]  = "SELECT     * FROM job WHERE    id > ? ORDER BY id ASC LIMIT 1;",

    [JOB_SET_RUN]      = "UPDATE job SET state =  'run', exec_started = ?                 WHERE id = ? AND state = 'pend';",
    [JOB_SET_ERROR]    = "UPDATE job SET state = 'fail', error        = ?, exec_ended = ? WHERE id = ?;",
    [JOB_SET_DONE]     = "UPDATE job SET state = 'done', exec_ended   = ?                 WHERE id = ?;",
    [JOB_INC_PROGRESS] = "UPDATE job SET progress = MIN(progress + ?, 100)                WHERE id = ?;",

    [JOB_DELETE_BY_ID] = "DELETE FROM job WHERE id = ?;",
    [JOB_DELETE]       = "DELETE FROM job;",

    /* --- SCAN queries --- */
    [SCAN_INSERT] = "INSERT INTO scan (db_id, multi_hits, hmmer3_compat, job_id) "
                    "VALUES           (    ?,          ?,             ?,      ?);",

    [SCAN_GET_BY_ID]     = "SELECT     * FROM scan WHERE     id = ?;",
    [SCAN_GET_BY_JOB_ID] = "SELECT     * FROM scan WHERE job_id = ?;",
    [SCAN_GET_NEXT]      = "SELECT     * FROM scan WHERE     id > ? ORDER BY id ASC LIMIT 1;",

    [SCAN_DELETE] = "DELETE FROM scan;",

    /* --- PROD queries --- */
    [PROD_INSERT] = "INSERT INTO prod (scan_id, seq_id, profile_name, abc_name, alt_loglik, null_loglik, evalue_log, profile_typeid, version, match) "
                    "VALUES           (      ?,      ?,            ?,        ?,          ?,           ?,              ?,       ?,     ?);",

    [PROD_GET]           = "SELECT  * FROM prod WHERE id = ?;",
    [PROD_GET_NEXT]      = "SELECT id FROM prod WHERE id > ? ORDER BY id ASC LIMIT 1;",
    [PROD_GET_SCAN_NEXT] = "SELECT id FROM prod WHERE id > ? AND scan_id = ? ORDER BY id ASC LIMIT 1;",

    [PROD_DELETE] = "DELETE FROM prod;",

    /* --- SEQ queries --- */
    [SEQ_INSERT] = "INSERT INTO seq (scan_id, name, data) VALUES (?, ?, ?);",

    [SEQ_GET]           = "SELECT id, scan_id, name, upper(data) FROM seq WHERE id = ?;",
    [SEQ_GET_NEXT]      = "SELECT id                             FROM seq WHERE id > ? ORDER BY id ASC LIMIT 1;",
    [SEQ_GET_SCAN_NEXT] = "SELECT id                             FROM seq WHERE id > ? AND scan_id = ? ORDER BY id ASC LIMIT 1;",

    [SEQ_DELETE] = "DELETE FROM seq;",
};
static_assert(ARRAY_SIZE(queries) == SEQ_DELETE + 1, "Cover all enum cases");
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

static struct xsql_stmt stmt[ARRAY_SIZE(queries)] = {0};

enum sched_rc stmt_init(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        stmt[i].st = stmts[i];
        stmt[i].query = queries[i];

        if (xsql_prepare(stmt + i)) return error(SCHED_FAIL_PREPARE_STMT);
    }
    return SCHED_OK;
}

struct xsql_stmt *stmt_get(int idx) { return stmt + idx; }

void stmt_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmt); ++i)
        xsql_finalize(stmt[i].st);
}
