#include "sched/rc.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

#define NOT_AN_ERROR "not an error"

static char const strings[][40] = {
    [SCHED_OK] = NOT_AN_ERROR,
    [SCHED_END] = NOT_AN_ERROR,
    [SCHED_NOT_FOUND] = NOT_AN_ERROR,
    [SCHED_NOT_ENOUGH_MEMORY] = "not enough memory",
    [SCHED_FAIL_PARSE_FILE] = "failed to parse file",
    [SCHED_FAIL_OPEN_FILE] = "failed to open file",
    [SCHED_FAIL_CLOSE_FILE] = "failed to close file",
    [SCHED_FAIL_WRITE_FILE] = "failed to write file",
    [SCHED_FAIL_READ_FILE] = "failed to read file",
    [SCHED_FAIL_REMOVE_FILE] = "failed to remove file",
    [SCHED_INVALID_FILE_NAME] = "invalid file name",
    [SCHED_INVALID_FILE_NAME_EXT] = "invalid file name extension",
    [SCHED_TOO_SHORT_FILE_NAME] = "file name is too short",
    [SCHED_TOO_LONG_FILE_NAME] = "file name is too long",
    [SCHED_TOO_LONG_FILE_PATH] = "file path is too long",
    [SCHED_FILE_NAME_NOT_SET] = "file name has not been set",
    [SCHED_HMM_ALREADY_EXISTS] = "hmm already exists",
    [SCHED_DB_ALREADY_EXISTS] = "database already exists",
    [SCHED_ASSOC_HMM_NOT_FOUND] = "associated hmm not found",
    [SCHED_FAIL_BIND_STMT] = "failed to bind value to sql statement",
    [SCHED_FAIL_EVAL_STMT] = "failed to evaluate sql statememt",
    [SCHED_FAIL_GET_FRESH_STMT] = "failed to get fresh sql statement",
    [SCHED_FAIL_GET_COLUMN_TEXT] = "failed to get column text",
    [SCHED_FAIL_EXEC_STMT] = "failed to execute sql statement",
    [SCHED_FAIL_PREPARE_STMT] = "failed to prepare sql statement",
    [SCHED_FAIL_FINALIZE_STMT] = "failed to finalize sql statement",
    [SCHED_FAIL_RESET_STMT] = "failed to reset sql statement",
    [SCHED_FAIL_OPEN_SCHED_FILE] = "failed to open sched file",
    [SCHED_FAIL_CLOSE_SCHED_FILE] = "failed to close sched file",
    [SCHED_SQLITE3_NOT_THREAD_SAFE] = "sqlite3 is not thread safe",
    [SCHED_SQLITE3_TOO_OLD] = "sqlite3 is too old",
    [SCHED_FAIL_BEGIN_TRANSACTION] = "failed to begin sql transaction",
    [SCHED_FAIL_END_TRANSACTION] = "failed to end sql transaction",
    [SCHED_FAIL_ROLLBACK_TRANSACTION] = "failed to rollback sql transaction"};

enum sched_rc __error_print(enum sched_rc rc, char const *ctx, char const *msg)
{
#ifdef NDEBUG
    (void)ctx;
    (void)msg;
#else
    fputs(ctx, stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    fflush(stderr);
#endif
    return rc;
}

char const *sched_error_string(enum sched_rc rc)
{
    return rc > SCHED_LAST_RC ? NOT_AN_ERROR : strings[rc];
}
