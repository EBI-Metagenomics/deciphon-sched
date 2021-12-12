#include "xsql.h"
#include "array.h"
#include "dcp_sched/rc.h"
#include <assert.h>
#include <safe.h>
#include <sqlite3.h>
#include <stdlib.h>

int xsql_txt_as_array(struct xsql_txt const *txt, struct array **arr)
{
    *arr = array_put(*arr, txt->str, (size_t)(txt->len + 1));
    if (!*arr) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val)
{
    assert(col >= 0);
    if (sqlite3_bind_double(stmt, col + 1, val)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val)
{
    assert(col >= 0);
    if (sqlite3_bind_int64(stmt, col + 1, val)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, str, -1, SQLITE_TRANSIENT))
        return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_bind_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, txt.str, (int)txt.len,
                          SQLITE_TRANSIENT))
        return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_get_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt *txt)
{
    txt->str = (char const *)sqlite3_column_text(stmt, col);
    if (!txt->str) return SCHED_FAIL;
    int ilen = sqlite3_column_bytes(stmt, col);
    assert(ilen > 0);
    txt->len = (unsigned)ilen;
    return SCHED_DONE;
}

int xsql_cpy_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return SCHED_FAIL;
    sqlite3_column_bytes(stmt, col);
    safe_strcpy((char *)txt.str, str, txt.len + 1);
    return SCHED_DONE;
}

int xsql_open(char const *filepath, struct sqlite3 **db)
{
    if (sqlite3_open(filepath, db)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_close(struct sqlite3 *db)
{
    if (sqlite3_close(db)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_exec(struct sqlite3 *db, char const *sql, xsql_callback cb,
              void *cb_arg)
{
    if (sqlite3_exec(db, sql, cb, cb_arg, 0)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_begin_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "BEGIN TRANSACTION;", 0, 0);
}

int xsql_end_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "END TRANSACTION;", 0, 0);
}

int xsql_rollback_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "ROLLBACK TRANSACTION;", 0, 0);
}

int xsql_prepare(struct sqlite3 *db, char const *sql,
                 struct sqlite3_stmt **stmt)
{
    if (sqlite3_prepare_v2(db, sql, -1, stmt, 0)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_reset(struct sqlite3_stmt *stmt)
{
    if (sqlite3_reset(stmt)) return SCHED_FAIL;
    return SCHED_DONE;
}

int xsql_insert_step(struct sqlite3_stmt *stmt)
{
    if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL;
    return xsql_end_step(stmt);
}

int xsql_step(struct sqlite3_stmt *stmt)
{
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return SCHED_DONE;
    if (code == SQLITE_ROW) return SCHED_NEXT;
    return SCHED_FAIL;
}

int xsql_end_step(struct sqlite3_stmt *stmt)
{
    if (sqlite3_step(stmt) != SQLITE_DONE) return SCHED_FAIL;
    return SCHED_DONE;
}
