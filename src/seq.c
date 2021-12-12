#include "seq.h"
#include "compiler.h"
#include "dcp_sched/rc.h"
#include "safe.h"
#include "sched.h"
#include "xsql.h"
#include <assert.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdint.h>

extern struct sqlite3 *sched;

enum
{
    INSERT,
    SELECT,
    SELECT_NEXT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO seq\
            (\
                job_id, name, data\
            )\
        VALUES\
            (\
                ?, ?, ?\
            ) RETURNING id;\
",
    [SELECT] = "SELECT id, job_id, name, upper(data) FROM seq WHERE id = ?;\
",
    [SELECT_NEXT] = \
"\
        SELECT\
            id FROM seq\
        WHERE\
            id > ? AND job_id = ? ORDER BY id ASC LIMIT 1;\
"};
/* clang-format on */

static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

int seq_module_init(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if (xsql_prepare(sched, queries[i], stmts + i)) return SCHED_FAIL;
    }
    return SCHED_DONE;
}

void sched_seq_init(struct sched_seq *seq, int64_t job_id, char const *name,
                    char const *data)
{
    seq->id = 0;
    seq->job_id = job_id;
    safe_strcpy(seq->name, name, ARRAY_SIZE_OF(*seq, name));
    safe_strcpy(seq->data, data, ARRAY_SIZE_OF(*seq, data));
}

int seq_submit(struct sched_seq *seq)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, seq->job_id)) return SCHED_FAIL;
    if (xsql_bind_str(stmt, 1, seq->name)) return SCHED_FAIL;
    if (xsql_bind_str(stmt, 2, seq->data)) return SCHED_FAIL;

    if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL;
    seq->id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

static int next_seq_id(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3_stmt *stmt = stmts[SELECT_NEXT];
    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, *seq_id)) return SCHED_FAIL;
    if (xsql_bind_i64(stmt, 1, job_id)) return SCHED_FAIL;

    int rc = xsql_step(stmt);
    if (rc == SCHED_DONE) return SCHED_NOTFOUND;
    if (rc != SCHED_NEXT) return SCHED_FAIL;
    *seq_id = sqlite3_column_int64(stmt, 0);

    return xsql_end_step(stmt);
}

static int get_seq(struct sched_seq *seq)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, seq->id)) return SCHED_FAIL;

    if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL;

    seq->id = sqlite3_column_int64(stmt, 0);
    seq->job_id = sqlite3_column_int64(stmt, 1);

    if (xsql_cpy_txt(stmt, 2, XSQL_TXT_OF(*seq, name))) return SCHED_FAIL;
    if (xsql_cpy_txt(stmt, 3, XSQL_TXT_OF(*seq, data))) return SCHED_FAIL;

    return xsql_end_step(stmt);
}

int sched_seq_next(struct sched_seq *seq)
{
    int rc = next_seq_id(seq->job_id, &seq->id);
    if (rc == SCHED_NOTFOUND) return SCHED_DONE;
    if (rc != SCHED_DONE) return SCHED_FAIL;
    if (get_seq(seq)) return SCHED_FAIL;
    return SCHED_NEXT;
}

void seq_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
