#ifndef STMT_H
#define STMT_H

#include "sched/rc.h"

enum stmt
{
    HMM_INSERT,
    HMM_GET_BY_ID,
    HMM_GET_BY_XXH3,
    HMM_GET_BY_FILENAME,
    HMM_DELETE,
    DB_INSERT,
    DB_GET_BY_ID,
    DB_GET_BY_XXH3,
    DB_GET_BY_FILENAME,
    DB_GET_NEXT,
    DB_DELETE,
    JOB_INSERT,
    JOB_GET_PEND,
    JOB_GET_STATE,
    JOB_GET,
    JOB_SET_RUN,
    JOB_SET_ERROR,
    JOB_SET_DONE,
    JOB_DELETE,
    SCAN_INSERT,
    SCAN_GET_BY_SCAN_ID,
    SCAN_GET_BY_JOB_ID,
    SCAN_DELETE,
    PROD_INSERT,
    PROD_GET,
    PROD_GET_NEXT,
    PROD_DELETE,
    SEQ_INSERT,
    SEQ_GET,
    SEQ_GET_NEXT,
    SEQ_DELETE,
};

struct sqlite3_stmt;
struct xsql_stmt;

enum sched_rc stmt_init(void);
struct xsql_stmt *stmt_get(int idx);
void stmt_del(void);

#endif
