#include "seq.h"
#include "error.h"
#include "sched/rc.h"
#include "sched/seq.h"
#include "stmt.h"
#include "xsql.h"
#include "xstrcpy.h"

void sched_seq_init(struct sched_seq *seq, int64_t seq_id, int64_t scan_id,
                    char const *name, char const *data)
{
    seq->id = seq_id;
    seq->scan_id = scan_id;
    XSTRCPY(seq, name, name);
    XSTRCPY(seq, data, data);
}

static void seq_init(struct sched_seq *seq)
{
    sched_seq_init(seq, 0, 0, "", "");
}

enum sched_rc seq_submit(struct sched_seq *seq)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SEQ_INSERT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, seq->scan_id)) return EBIND;
    if (xsql_bind_str(st, 1, seq->name)) return EBIND;
    if (xsql_bind_str(st, 2, seq->data)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    seq->id = xsql_last_id();
    return SCHED_OK;
}

enum sched_rc seq_wipe(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SEQ_DELETE));
    if (!st) return EFRESH;

    enum sched_rc rc = xsql_step(st);
    return rc == SCHED_END ? SCHED_OK : ESTEP;
}

static enum sched_rc next_seq_scan_id(int64_t scan_id, int64_t *seq_id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SEQ_GET_SCAN_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, *seq_id)) return EBIND;
    if (xsql_bind_i64(st, 1, scan_id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_SEQ_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;
    *seq_id = xsql_get_i64(st, 0);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

static enum sched_rc next_seq_id(int64_t *seq_id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SEQ_GET_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, *seq_id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_SEQ_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;
    *seq_id = xsql_get_i64(st, 0);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_seq_get_by_id(struct sched_seq *seq, int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SEQ_GET));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_SEQ_NOT_FOUND;
    if (rc != SCHED_OK) ESTEP;

    seq->id = xsql_get_i64(st, 0);
    seq->scan_id = xsql_get_i64(st, 1);

    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*seq, name))) return EGETTXT;
    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*seq, data))) return EGETTXT;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_seq_scan_next(struct sched_seq *seq)
{
    enum sched_rc rc = next_seq_scan_id(seq->scan_id, &seq->id);
    if (rc == SCHED_SEQ_NOT_FOUND) return SCHED_SEQ_NOT_FOUND;
    if (rc != SCHED_OK) return rc;
    return sched_seq_get_by_id(seq, seq->id);
}

static enum sched_rc next_seq(struct sched_seq *seq)
{
    enum sched_rc rc = next_seq_id(&seq->id);
    if (rc == SCHED_SEQ_NOT_FOUND) return SCHED_SEQ_NOT_FOUND;
    if (rc != SCHED_OK) return rc;
    return sched_seq_get_by_id(seq, seq->id);
}

enum sched_rc sched_seq_get_all(sched_seq_set_func_t fn, struct sched_seq *seq,
                                void *arg)
{
    enum sched_rc rc = SCHED_OK;

    seq_init(seq);
    while ((rc = next_seq(seq)) == SCHED_OK)
        fn(seq, arg);
    return rc == SCHED_SEQ_NOT_FOUND ? SCHED_OK : rc;
}
