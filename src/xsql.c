#include "xsql.h"
#include "sched/rc.h"
#include "sqlite3/sqlite3.h"
#include "strlcpy.h"
#include <assert.h>
#include <stdlib.h>

enum sched_rc xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val)
{
    assert(col >= 0);
    if (sqlite3_bind_double(stmt, col + 1, val)) return SCHED_EFAIL;
    return SCHED_OK;
}

enum sched_rc xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val)
{
    assert(col >= 0);
    if (sqlite3_bind_int64(stmt, col + 1, val)) return SCHED_EFAIL;
    return SCHED_OK;
}

enum sched_rc xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, str, -1, SQLITE_TRANSIENT))
        return SCHED_EFAIL;
    return SCHED_OK;
}

enum sched_rc xsql_bind_txt(struct sqlite3_stmt *stmt, int col,
                            struct xsql_txt txt)
{
    assert(col >= 0);
    if (sqlite3_bind_text(stmt, col + 1, txt.str, (int)txt.len,
                          SQLITE_TRANSIENT))
        return SCHED_EFAIL;
    return SCHED_OK;
}

enum sched_rc xsql_cpy_txt(struct sqlite3_stmt *stmt, int col,
                           struct xsql_txt txt)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return SCHED_EFAIL;
    sqlite3_column_bytes(stmt, col);
    strlcpy((char *)txt.str, str, txt.len + 1);
    return SCHED_OK;
}

enum sched_rc xsql_open(char const *filepath, struct sqlite3 **db)
{
    if (sqlite3_open(filepath, db)) return SCHED_EFAIL;
    if (xsql_exec(*db, "PRAGMA foreign_keys = ON;", 0, 0))
    {
        sqlite3_close(*db);
        return SCHED_EFAIL;
    }
    return SCHED_OK;
}

enum sched_rc xsql_close(struct sqlite3 *db)
{
    if (sqlite3_close(db)) return SCHED_EFAIL;
    return SCHED_OK;
}

enum sched_rc xsql_exec(struct sqlite3 *db, char const *sql, xsql_callback cb,
                        void *cb_arg)
{
    if (sqlite3_exec(db, sql, cb, cb_arg, 0)) return SCHED_EFAIL;
    return SCHED_OK;
}

enum sched_rc xsql_begin_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "BEGIN TRANSACTION;", 0, 0);
}

enum sched_rc xsql_end_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "END TRANSACTION;", 0, 0);
}

enum sched_rc xsql_rollback_transaction(struct sqlite3 *db)
{
    return xsql_exec(db, "ROLLBACK TRANSACTION;", 0, 0);
}

enum sched_rc xsql_prepare(struct sqlite3 *db, struct xsql_stmt *stmt)
{
    return sqlite3_prepare_v2(db, stmt->query, -1, &stmt->st, 0) ? SCHED_EFAIL
                                                                 : SCHED_OK;
}

static enum sched_rc reset(struct sqlite3_stmt *stmt)
{
    int code = sqlite3_reset(stmt);
    if (code == SQLITE_CONSTRAINT) return SCHED_EINVAL;
    if (code) return SCHED_EFAIL;
    return SCHED_OK;
}

struct sqlite3_stmt *xsql_fresh_stmt(struct sqlite3 *db, struct xsql_stmt *stmt)
{
    int code = sqlite3_reset(stmt->st);
    if (code == SQLITE_CONSTRAINT)
    {
        if (sqlite3_finalize(stmt->st)) return 0;
        if (sqlite3_prepare_v2(db, stmt->query, -1, &stmt->st, 0)) return 0;
        return reset(stmt->st) ? 0 : stmt->st;
    }
    return code ? 0 : stmt->st;
}

enum sched_rc xsql_step(struct sqlite3_stmt *stmt)
{
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return SCHED_END;
    if (code == SQLITE_ROW) return SCHED_OK;
    if (code == SQLITE_CONSTRAINT) return SCHED_EINVAL;
    return SCHED_EFAIL;
}

enum sched_rc xsql_finalize(struct sqlite3_stmt *stmt)
{
    return sqlite3_finalize(stmt) ? SCHED_EFAIL : SCHED_OK;
}

int64_t xsql_last_id(struct sqlite3 *db)
{
    return sqlite3_last_insert_rowid(db);
}
