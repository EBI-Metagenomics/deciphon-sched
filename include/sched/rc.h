#ifndef SCHED_RC_H
#define SCHED_RC_H

enum sched_rc
{
    SCHED_OK,
    SCHED_END,
    SCHED_HMM_NOT_FOUND,
    SCHED_SCAN_NOT_FOUND,
    SCHED_DB_NOT_FOUND,
    SCHED_JOB_NOT_FOUND,
    SCHED_PROD_NOT_FOUND,
    SCHED_SEQ_NOT_FOUND,
    SCHED_HMMER_NOT_FOUND,
    SCHED_NOT_ENOUGH_MEMORY,
    SCHED_FAIL_PARSE_FILENAME,
    SCHED_FAIL_PARSE_FILE,
    SCHED_FAIL_OPEN_FILE,
    SCHED_FAIL_CLOSE_FILE,
    SCHED_FAIL_WRITE_FILE,
    SCHED_FAIL_READ_FILE,
    SCHED_FAIL_STAT_FILE,
    SCHED_FAIL_REMOVE_FILE,
    SCHED_INVALID_FILE_NAME,
    SCHED_INVALID_FILE_NAME_EXT,
    SCHED_TOO_SHORT_FILE_NAME,
    SCHED_TOO_LONG_FILE_NAME,
    SCHED_TOO_LONG_FILE_PATH,
    SCHED_FILE_NAME_NOT_SET,
    SCHED_HMM_ALREADY_EXISTS,
    SCHED_DB_ALREADY_EXISTS,
    SCHED_ASSOC_HMM_NOT_FOUND,
    SCHED_TOO_LONG_PROFNAME,
    SCHED_FAIL_BIND_STMT,
    SCHED_FAIL_EVAL_STMT,
    SCHED_FAIL_GET_FRESH_STMT,
    SCHED_FAIL_GET_COLUMN_TEXT,
    SCHED_FAIL_GET_COLUMN_BLOB,
    SCHED_FAIL_EXEC_STMT,
    SCHED_FAIL_PREPARE_STMT,
    SCHED_FAIL_RESET_STMT,
    SCHED_FAIL_OPEN_SCHED_FILE,
    SCHED_FAIL_CLOSE_SCHED_FILE,
    SCHED_SQLITE3_NOT_THREAD_SAFE,
    SCHED_SQLITE3_TOO_OLD,
    SCHED_FAIL_BEGIN_TRANSACTION,
    SCHED_FAIL_END_TRANSACTION,
    SCHED_FAIL_ROLLBACK_TRANSACTION,
};

#define SCHED_LAST_RC SCHED_FAIL_ROLLBACK_TRANSACTION

#endif
