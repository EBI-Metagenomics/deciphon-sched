#ifndef XSQL_H
#define XSQL_H

#include "compiler.h"
#include <inttypes.h>
#include <stdbool.h>

typedef int(xsql_callback)(void *, int, char **, char **);

struct sqlite3;
struct sqlite3_stmt;

struct xsql_txt
{
    unsigned len;
    char const *str;
};

#define XSQL_TXT_OF(var, member)                                               \
    (struct xsql_txt) { ARRAY_SIZE_OF((var), member) - 1, (var).member }

int xsql_bind_dbl(struct sqlite3_stmt *stmt, int col, double val);
int xsql_bind_i64(struct sqlite3_stmt *stmt, int col, int64_t val);
int xsql_bind_str(struct sqlite3_stmt *stmt, int col, char const *str);
int xsql_bind_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt);

int xsql_get_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt *txt);
int xsql_cpy_txt(struct sqlite3_stmt *stmt, int col, struct xsql_txt txt);

int xsql_open(char const *filepath, struct sqlite3 **db);
int xsql_close(struct sqlite3 *db);
int xsql_exec(struct sqlite3 *db, char const *, xsql_callback, void *);

int xsql_begin_transaction(struct sqlite3 *db);
int xsql_end_transaction(struct sqlite3 *db);
int xsql_rollback_transaction(struct sqlite3 *db);

int xsql_prepare(struct sqlite3 *db, char const *sql,
                 struct sqlite3_stmt **stmt);
int xsql_reset(struct sqlite3_stmt *stmt);
int xsql_insert_step(struct sqlite3_stmt *stmt);
int xsql_step(struct sqlite3_stmt *stmt);
int xsql_end_step(struct sqlite3_stmt *stmt);

#endif
