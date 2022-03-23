#ifndef STMT_H
#define STMT_H

#include "sched/rc.h"
#include "xsql.h"

enum stmt
{
    DB_INSERT,
    DB_SELECT_BY_ID,
    DB_SELECT_BY_XXH3,
    DB_SELECT_BY_FILENAME,
    DB_SELECT_NEXT,
    DB_DELETE,
    JOB_INSERT,
    JOB_GET_PEND,
    JOB_GET_STATE,
    JOB_SELECT,
    JOB_SET_RUN,
    JOB_SET_ERROR,
    JOB_SET_DONE,
    JOB_DELETE,
    SCAN_INSERT,
    SCAN_GET_BY_SCAN_ID,
    SCAN_GET_BY_JOB_ID,
    SCAN_DELETE,
    PROD_INSERT,
    PROD_SELECT,
    PROD_SELECT_NEXT,
    PROD_DELETE,
    SEQ_INSERT,
    SEQ_SELECT,
    SEQ_SELECT_NEXT,
    SEQ_DELETE,
};

struct sqlite3_stmt;
struct xsql_stmt;

enum sched_rc stmt_init(void);
struct xsql_stmt *stmt_get(int idx);
void stmt_del(void);

#endif
