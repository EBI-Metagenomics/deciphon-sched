#include "seq.h"
#include "logger.h"
#include "sched.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "stmt.h"
#include "xsql.h"
#include "xstrcpy.h"

void sched_seq_init(struct sched_seq *seq, int64_t scan_id, char const *name,
                    char const *data)
{
    seq->id = 0;
    seq->scan_id = scan_id;
    XSTRCPY(seq, name, name);
    XSTRCPY(seq, data, data);
}

enum sched_rc seq_submit(struct sched_seq *seq)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_INSERT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, seq->scan_id)) return EBIND;
    if (xsql_bind_str(st, 1, seq->name)) return EBIND;
    if (xsql_bind_str(st, 2, seq->data)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    seq->id = xsql_last_id(sched);
    return SCHED_OK;
}

enum sched_rc seq_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete db");
}

static int next_seq_id(int64_t scan_id, int64_t *seq_id)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_SELECT_NEXT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, *seq_id)) return EBIND;
    if (xsql_bind_i64(st, 1, scan_id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get next seq id");
    *seq_id = xsql_get_i64(st, 0);

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc sched_seq_get_by_id(struct sched_seq *seq, int64_t id)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SEQ_SELECT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get seq");

    seq->id = xsql_get_i64(st, 0);
    seq->scan_id = xsql_get_i64(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*seq, name))) return ECPYTXT;
    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*seq, data))) return ECPYTXT;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

enum sched_rc sched_seq_next(struct sched_seq *seq)
{
    enum sched_rc rc = next_seq_id(seq->scan_id, &seq->id);
    if (rc == SCHED_NOTFOUND) return SCHED_END;
    if (rc != SCHED_OK) return rc;
    return sched_seq_get_by_id(seq, seq->id);
}
