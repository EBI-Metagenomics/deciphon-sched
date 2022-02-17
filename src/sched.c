#include "sched/sched.h"
#include "bug.h"
#include "compiler.h"
#include "db.h"
#include "job.h"
#include "logger.h"
#include "prod.h"
#include "sched/rc.h"
#include "schema.h"
#include "seq.h"
#include "seq_queue.h"
#include "stmt.h"
#include "strlcpy.h"
#include "utc.h"
#include "xfile.h"
#include "xsql.h"
#include <assert.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct sqlite3 *sched = NULL;
char sched_filepath[PATH_SIZE] = {0};

#define MIN_SQLITE_VERSION 3031001

static_assert(SQLITE_VERSION_NUMBER >= MIN_SQLITE_VERSION,
              "Minimum sqlite requirement.");

enum sched_rc emerge_db(char const *filepath);
enum sched_rc is_empty(char const *filepath, bool *empty);
// enum sched_rc touch_db(char const *filepath);

enum sched_rc sched_setup(char const *filepath)
{
    strlcpy(sched_filepath, filepath, ARRAY_SIZE(sched_filepath));

    int thread_safe = sqlite3_threadsafe();
    if (thread_safe == 0) return efail("not thread safe");
    if (sqlite3_libversion_number() < MIN_SQLITE_VERSION)
        return efail("old sqlite3");

    enum sched_rc rc = xfile_touch(filepath);
    if (rc) return rc;

    bool empty = false;
    if (is_empty(filepath, &empty)) BUG();

    if (empty && emerge_db(filepath)) return efail("emerge db");

    return SCHED_OK;
}

enum sched_rc sched_open(void)
{
    if (xsql_open(sched_filepath, &sched)) goto cleanup;
    if (stmt_init()) goto cleanup;

    return SCHED_OK;

cleanup:
    xsql_close(sched);
    return SCHED_EFAIL;
}

enum sched_rc sched_close(void)
{
    stmt_del();
    return xsql_close(sched);
}

enum sched_rc sched_job_next_pend(struct sched_job *job)
{
    return job_next_pend(job);
}

enum sched_rc sched_next_pending_job(struct sched_job *job)
{
    return job_next_pending(job);
}

enum sched_rc emerge_db(char const *filepath)
{
    int rc = 0;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;

    if ((rc = xsql_exec(db, (char const *)schema, 0, 0))) goto cleanup;

    return xsql_close(db);

cleanup:
    xsql_close(sched);
    return rc;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

enum sched_rc is_empty(char const *filepath, bool *empty)
{
    int rc = 0;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if ((rc = xsql_exec(db, sql, is_empty_cb, empty))) goto cleanup;

    return xsql_close(db);

cleanup:
    xsql_close(sched);
    return rc;
}
