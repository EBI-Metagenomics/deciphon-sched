#ifndef ERROR_H
#define ERROR_H

#include "compiler.h"
#include "sched/error.h"

enum sched_rc __error_print(enum sched_rc rc, char const *ctx, char const *msg);
#define error(rc) __error_print(rc, LOCAL, sched_error_string(rc))

#define EFRESH error(SCHED_FAIL_GET_FRESH_STMT)
#define ESTEP error(SCHED_FAIL_EVAL_STMT)
#define EGETTXT error(SCHED_FAIL_GET_COLUMN_TEXT)
#define EGETBLOB error(SCHED_FAIL_GET_COLUMN_BLOB)
#define EBIND error(SCHED_FAIL_BIND_STMT)
#define EEXEC error(SCHED_FAIL_EXEC_STMT)
#define EBEGINSTMT error(SCHED_FAIL_BEGIN_TRANSACTION)
#define EENDSTMT error(SCHED_FAIL_END_TRANSACTION)
#define EROLLBACK error(SCHED_FAIL_END_TRANSACTION)

#define EWRITEFILE error(SCHED_FAIL_WRITE_FILE)
#define EPARSEFILE error(SCHED_FAIL_PARSE_FILE)

#endif
