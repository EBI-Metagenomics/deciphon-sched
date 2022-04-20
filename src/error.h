#ifndef ERROR_H
#define ERROR_H

#include "compiler.h"

/* clang-format off */
#define error(code)                                                                     \
    code == SCHED_NOT_ENOUGH_MEMORY                                                     \
        ? __error(code, "not enough memory")                                            \
    : code == SCHED_FAIL_PARSE_FILE                                                     \
        ? __error(code, "failed to parse file")                                         \
    : code == SCHED_FAIL_OPEN_FILE                                                      \
        ? __error(code, "failed to open file")                                          \
    : code == SCHED_FAIL_CLOSE_FILE                                                     \
        ? __error(code, "failed to close file")                                         \
    : code == SCHED_FAIL_WRITE_FILE                                                     \
        ? __error(code, "failed to write file")                                         \
    : code == SCHED_FAIL_READ_FILE                                                      \
        ? __error(code, "failed to read file")                                          \
    : code == SCHED_FAIL_REMOVE_FILE                                                    \
        ? __error(code, "failed to remove file")                                        \
    : code == SCHED_INVALID_FILE_NAME                                                   \
        ? __error(code, "invalid file name")                                            \
    : code == SCHED_INVALID_FILE_NAME_EXT                                               \
        ? __error(code, "invalid file name extension")                                  \
    : code == SCHED_TOO_SHORT_FILE_NAME                                                 \
        ? __error(code, "file name is too short")                                       \
    : code == SCHED_TOO_LONG_FILE_NAME                                                  \
        ? __error(code, "file name is too long")                                        \
    : code == SCHED_TOO_LONG_FILE_PATH                                                  \
        ? __error(code, "file path is too long")                                        \
    : code == SCHED_FILE_NAME_NOT_SET                                                   \
        ? __error(code, "file name has not been set")                                   \
    : code == SCHED_HMM_ALREADY_EXISTS                                                  \
        ? __error(code, "hmm already exists")                                           \
    : code == SCHED_DB_ALREADY_EXISTS                                                   \
        ? __error(code, "database already exists")                                      \
    : code == SCHED_ASSOC_HMM_NOT_FOUND                                                 \
        ? __error(code, "associated hmm not found")                                     \
    : code == SCHED_FAIL_BIND_STMT                                                      \
        ? __error(code, "failed to bind value to sql statement")                        \
    : code == SCHED_FAIL_EVAL_STMT                                                      \
        ? __error(code, "failed to evaluate sql statememt")                             \
    : code == SCHED_FAIL_GET_FRESH_STMT                                                 \
        ? __error(code, "failed to get fresh sql statement")                            \
    : code == SCHED_FAIL_GET_COLUMN_TEXT                                                \
        ? __error(code, "failed to get column text")                                    \
    : code == SCHED_FAIL_EXEC_STMT                                                      \
        ? __error(code, "failed to execute sql statement")                              \
    : code == SCHED_FAIL_PREPARE_STMT                                                   \
        ? __error(code, "failed to prepare sql statement")                              \
    : code == SCHED_FAIL_FINALIZE_STMT                                                  \
        ? __error(code, "failed to finalize sql statement")                             \
    : code == SCHED_FAIL_RESET_STMT                                                     \
        ? __error(code, "failed to reset sql statement")                                \
    : code == SCHED_FAIL_OPEN_SCHED_FILE                                                \
        ? __error(code, "failed to open sched file")                                    \
    : code == SCHED_FAIL_CLOSE_SCHED_FILE                                               \
        ? __error(code, "failed to close sched file")                                   \
    : code == SCHED_SQLITE3_NOT_THREAD_SAFE                                             \
        ? __error(code, "sqlite3 is not thread safe")                                   \
    : code == SCHED_SQLITE3_TOO_OLD                                                     \
        ? __error(code, "sqlite3 is too old")                                           \
    : code == SCHED_FAIL_BEGIN_TRANSACTION                                              \
        ? __error(code, "failed to begin sql transaction")                              \
    : code == SCHED_FAIL_END_TRANSACTION                                                \
        ? __error(code, "failed to end sql transaction")                                \
    : code == SCHED_FAIL_ROLLBACK_TRANSACTION                                           \
        ? __error(code, "failed to rollback sql transaction")                           \
    : __error(code, "Not an error")
/* clang-format on */

#define EFRESH error(SCHED_FAIL_GET_FRESH_STMT)
#define ESTEP error(SCHED_FAIL_EVAL_STMT)
#define EGETTXT error(SCHED_FAIL_GET_COLUMN_TEXT)
#define EBIND error(SCHED_FAIL_BIND_STMT)
#define EEXEC error(SCHED_FAIL_EXEC_STMT)
#define EBEGINSTMT error(SCHED_FAIL_BEGIN_TRANSACTION)
#define EENDSTMT error(SCHED_FAIL_END_TRANSACTION)
#define EROLLBACK error(SCHED_FAIL_END_TRANSACTION)

#define EWRITEFILE error(SCHED_FAIL_WRITE_FILE)
#define EPARSEFILE error(SCHED_FAIL_PARSE_FILE)

#define __error(rc, msg) __error_print(rc, LOCAL, msg)

enum sched_rc __error_print(enum sched_rc rc, char const *ctx, char const *msg);

#endif
