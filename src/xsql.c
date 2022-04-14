#include "xsql.h"
#include "logger.h"
#include "sched/rc.h"
#include "sqlite3/sqlite3.h"
#include "xstrcpy.h"
#include <assert.h>
#include <stdlib.h>

static_assert(SQLITE_VERSION_NUMBER >= XSQL_REQUIRED_VERSION,
              "Minimum sqlite requirement.");

static struct sqlite3 *sched = NULL;

bool xsql_is_thread_safe(void) { return sqlite3_threadsafe(); }

int xsql_version(void) { return sqlite3_libversion_number(); }

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

int xsql_get_int(struct sqlite3_stmt *stmt, int col)
{
    return sqlite3_column_int(stmt, col);
}

int64_t xsql_get_i64(struct sqlite3_stmt *stmt, int col)
{
    return sqlite3_column_int64(stmt, col);
}

double xsql_get_dbl(struct sqlite3_stmt *stmt, int col)
{
    return sqlite3_column_double(stmt, col);
}

enum sched_rc xsql_cpy_txt(struct sqlite3_stmt *stmt, int col,
                           struct xsql_txt txt)
{
    char const *str = (char const *)sqlite3_column_text(stmt, col);
    if (!str) return SCHED_EFAIL;
    sqlite3_column_bytes(stmt, col);
    return xstrcpy((char *)txt.str, str, txt.len + 1) ? SCHED_OK : SCHED_EFAIL;
}

enum sched_rc xsql_open(char const *filepath)
{
    if (sqlite3_open(filepath, &sched)) return SCHED_EFAIL;
    if (xsql_exec("PRAGMA foreign_keys = ON;", 0, 0))
    {
        sqlite3_close(sched);
        return SCHED_EFAIL;
    }
    return SCHED_OK;
}

enum sched_rc xsql_close(void)
{
    return sqlite3_close(sched) ? SCHED_EFAIL : SCHED_OK;
}

enum sched_rc xsql_exec(char const *sql, xsql_func_t fn, void *arg)
{
    return sqlite3_exec(sched, sql, fn, arg, 0) ? SCHED_EFAIL : SCHED_OK;
}

enum sched_rc xsql_begin_transaction(void)
{
    return xsql_exec("BEGIN TRANSACTION;", 0, 0);
}

enum sched_rc xsql_end_transaction(void)
{
    return xsql_exec("END TRANSACTION;", 0, 0);
}

enum sched_rc xsql_rollback_transaction(void)
{
    return xsql_exec("ROLLBACK TRANSACTION;", 0, 0);
}

enum sched_rc xsql_prepare(struct xsql_stmt *stmt)
{
    return sqlite3_prepare_v2(sched, stmt->query, -1, &stmt->st, 0)
               ? SCHED_EFAIL
               : SCHED_OK;
}

static enum sched_rc reset(struct sqlite3_stmt *stmt)
{
    int code = sqlite3_reset(stmt);
    if (code == SQLITE_CONSTRAINT) return SCHED_EINVAL;
    if (code) return SCHED_EFAIL;
    return SCHED_OK;
}

struct sqlite3_stmt *xsql_fresh_stmt(struct xsql_stmt *stmt)
{
    int code = sqlite3_reset(stmt->st);
    if (code == SQLITE_CONSTRAINT)
    {
        if (sqlite3_finalize(stmt->st)) return 0;
        if (sqlite3_prepare_v2(sched, stmt->query, -1, &stmt->st, 0)) return 0;
        return reset(stmt->st) ? 0 : stmt->st;
    }
    return code ? 0 : stmt->st;
}

enum sched_rc xsql_step(struct sqlite3_stmt *stmt)
{
    int code = sqlite3_step(stmt);
    if (code == SQLITE_DONE) return SCHED_END;
    if (code == SQLITE_ROW) return SCHED_OK;
    if (code == SQLITE_CONSTRAINT) return SCHED_ECONSTRAINT;
    return SCHED_EFAIL;
}

enum sched_rc xsql_finalize(struct sqlite3_stmt *stmt)
{
    return sqlite3_finalize(stmt) ? SCHED_EFAIL : SCHED_OK;
}

int xsql_changes(void) { return sqlite3_changes(sched); }

int64_t xsql_last_id(void) { return sqlite3_last_insert_rowid(sched); }
