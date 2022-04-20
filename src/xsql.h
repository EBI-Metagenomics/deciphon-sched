#ifndef XSQL_H
#define XSQL_H

#include <inttypes.h>
#include <stdbool.h>

#define XSQL_REQUIRED_VERSION 3031001

typedef int(xsql_func_t)(void *, int, char **, char **);

struct sqlite3;
struct sqlite3_stmt;

struct xsql_txt
{
    unsigned len;
    char const *str;
};

struct xsql_stmt
{
    struct sqlite3_stmt *st;
    char const *query;
};

#define XSQL_TXT_OF(var, member)                                               \
    (struct xsql_txt) { ARRAY_SIZE_OF((var), member) - 1, (var).member }

bool xsql_is_thread_safe(void);
int xsql_version(void);

enum sched_rc xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val);
enum sched_rc xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val);
enum sched_rc xsql_bind_str(struct sqlite3_stmt *stmt, int col,
                            char const *str);
enum sched_rc xsql_bind_txt(struct sqlite3_stmt *stmt, int col,
                            struct xsql_txt txt);

int xsql_get_int(struct sqlite3_stmt *stmt, int col);
int64_t xsql_get_i64(struct sqlite3_stmt *stmt, int col);
double xsql_get_dbl(struct sqlite3_stmt *stmt, int col);
enum sched_rc xsql_cpy_txt(struct sqlite3_stmt *stmt, int col,
                           struct xsql_txt txt);

enum sched_rc xsql_open(char const *filepath);
enum sched_rc xsql_close(void);
enum sched_rc xsql_exec(char const *, xsql_func_t, void *);

enum sched_rc xsql_begin_transaction(void);
enum sched_rc xsql_end_transaction(void);
enum sched_rc xsql_rollback_transaction(void);

enum sched_rc xsql_prepare(struct xsql_stmt *stmt);
struct sqlite3_stmt *xsql_fresh_stmt(struct xsql_stmt *stmt);
enum sched_rc xsql_step(struct sqlite3_stmt *stmt);
void xsql_finalize(struct sqlite3_stmt *stmt);
int xsql_changes(void);

int64_t xsql_last_id(void);

#endif
