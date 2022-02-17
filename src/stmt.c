#include "stmt.h"
#include "compiler.h"
#include "logger.h"
#include "xsql.h"
#include <sqlite3.h>

/* clang-format off */
static char const *const queries[] =
{
    [DB_INSERT] = "INSERT INTO db (xxh64, filename) VALUES (?, ?);",

    [DB_SELECT_BY_ID]       = "SELECT * FROM db WHERE       id = ?;",
    [DB_SELECT_BY_XXH64]    = "SELECT * FROM db WHERE    xxh64 = ?;",
    [DB_SELECT_BY_FILENAME] = "SELECT * FROM db WHERE filename = ?;",
    [DB_SELECT_NEXT]        = "SELECT * FROM db WHERE id > ? ORDER BY id ASC LIMIT 1;",

    [JOB_INSERT] = "INSERT INTO job (db_id, multi_hits, hmmer3_compat, state, error, submission, exec_started, exec_ended)"
                   "VALUES          (    ?,          ?,             ?,     ?,     ?,          ?,            ?,          ?);",

    [JOB_GET_PEND]  = "SELECT    id FROM job WHERE state = 'pend' ORDER BY id LIMIT 1;",
    [JOB_GET_STATE] = "SELECT state FROM job WHERE    id = ?;",
    [JOB_SELECT]    = "SELECT     * FROM job WHERE    id = ?;",

    [JOB_SET_RUN]   = "UPDATE job SET state =  'run', exec_started = ?                 WHERE id = ? AND state = 'pend';",
    [JOB_SET_ERROR] = "UPDATE job SET state = 'fail', error        = ?, exec_ended = ? WHERE id = ?;",
    [JOB_SET_DONE]  = "UPDATE job SET state = 'done', exec_ended   = ?                 WHERE id = ?;",

    [PROD_INSERT] = "INSERT INTO prod (job_id, seq_id, profile_name, abc_name, alt_loglik, null_loglik, profile_typeid, version, match)"
                    "VALUES           (     ?,      ?,            ?,        ?,          ?,           ?,              ?,       ?,     ?);",

    [PROD_SELECT]      = "SELECT  * FROM prod WHERE id = ?;",
    [PROD_SELECT_NEXT] = "SELECT id FROM prod WHERE id > ? AND job_id = ? ORDER BY id ASC LIMIT 1;",

    [SEQ_INSERT] = "INSERT INTO seq (job_id, name, data) VALUES (?, ?, ?);",

    [SEQ_SELECT]      = "SELECT id, job_id, name, upper(data) FROM seq WHERE id = ?;",
    [SEQ_SELECT_NEXT] = "SELECT id                            FROM seq WHERE id > ? AND job_id = ? ORDER BY id ASC LIMIT 1;"
};
/* clang-format on */

struct sqlite3_stmt *stmt[ARRAY_SIZE(queries)] = {0};
extern struct sqlite3 *sched;

enum sched_rc stmt_init(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if (xsql_prepare(sched, queries[i], stmt + i))
            return efail("prepare stmt");
    }
    return SCHED_OK;
}

void stmt_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmt); ++i)
        sqlite3_finalize(stmt[i]);
}
