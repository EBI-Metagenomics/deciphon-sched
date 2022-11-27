#include "sched/hmmer.h"
#include "error.h"
#include "hmmer.h"
#include "sched/rc.h"
#include "stmt.h"
#include "xsql.h"

static enum sched_rc select_hmmer_i64(struct sched_hmmer *hmmer,
                                      int64_t by_value, enum stmt select_stmt)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(select_stmt));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_HMMER_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;

    struct xsql_blob blob = {0};

    hmmer->id = xsql_get_i64(st, 0);
    if (xsql_cpy_blob(st, 1, &blob)) return EGETBLOB;
    hmmer->len = blob.len;
    hmmer->data = blob.data;
    hmmer->prod_id = xsql_get_i64(st, 2);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

void sched_hmmer_init(struct sched_hmmer *hmmer, int64_t prod_id)
{
    hmmer->id = 0;
    hmmer->len = 0;
    hmmer->data = 0;
    hmmer->prod_id = prod_id;
}

enum sched_rc sched_hmmer_get_by_id(struct sched_hmmer *hmmer, int64_t id)
{
    return select_hmmer_i64(hmmer, id, HMMER_GET_BY_ID);
}

enum sched_rc sched_hmmer_get_by_prod_id(struct sched_hmmer *hmmer,
                                         int64_t prod_id)
{
    return select_hmmer_i64(hmmer, prod_id, HMMER_GET_BY_PROD_ID);
}

enum sched_rc sched_hmmer_add(struct sched_hmmer *hmmer, int len,
                              unsigned char const *data)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMMER_INSERT));
    if (!st) return EFRESH;

    hmmer->len = len;
    hmmer->data = data;
    struct xsql_blob blob = {.len = len, .data = data};

    if (xsql_bind_blob(st, 0, blob)) return EBIND;
    if (xsql_bind_i64(st, 1, hmmer->prod_id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    hmmer->id = xsql_last_id();
    return SCHED_OK;
}

enum sched_rc sched_hmmer_remove(int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMMER_DELETE_BY_ID));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc != SCHED_END) return ESTEP;
    return xsql_changes() == 0 ? SCHED_HMMER_NOT_FOUND : SCHED_OK;
}

enum sched_rc hmmer_wipe(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMMER_DELETE));
    if (!st) return EFRESH;

    enum sched_rc rc = xsql_step(st);
    return rc == SCHED_END ? SCHED_OK : ESTEP;
}
