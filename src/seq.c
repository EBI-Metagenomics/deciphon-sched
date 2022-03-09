#include "seq.h"
#include "logger.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "sqlite3/sqlite3.h"
#include "stmt.h"
#include "strlcpy.h"
#include "xsql.h"

void sched_seq_init(struct sched_seq *seq, int64_t job_id, char const *name,
                    char const *data)
{
    seq->id = 0;
    seq->job_id = job_id;
    strlcpy(seq->name, name, ARRAY_SIZE_OF(*seq, name));
    strlcpy(seq->data, data, ARRAY_SIZE_OF(*seq, data));
}

enum sched_rc seq_submit(struct sched_seq *seq)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_INSERT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, seq->job_id)) return efail("bind");
    if (xsql_bind_str(st, 1, seq->name)) return efail("bind");
    if (xsql_bind_str(st, 2, seq->data)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    seq->id = xsql_last_id(sched);
    return SCHED_OK;
}

enum sched_rc seq_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete db");
}

static int next_seq_id(int64_t job_id, int64_t *seq_id)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_SELECT_NEXT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, *seq_id)) return efail("bind");
    if (xsql_bind_i64(st, 1, job_id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get next seq id");
    *seq_id = sqlite3_column_int64(st, 0);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc sched_seq_get(struct sched_seq *seq)
{
#define ecpy efail("copy txt")

    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_SELECT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, seq->id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get seq");

    seq->id = sqlite3_column_int64(st, 0);
    seq->job_id = sqlite3_column_int64(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*seq, name))) return ecpy;
    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*seq, data))) return ecpy;

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;

#undef ecpy
}

enum sched_rc sched_seq_next(struct sched_seq *seq)
{
    enum sched_rc rc = next_seq_id(seq->job_id, &seq->id);
    if (rc == SCHED_NOTFOUND) return SCHED_END;
    if (rc != SCHED_OK) return rc;
    return sched_seq_get(seq);
}
