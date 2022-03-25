#include "sched/sched.h"
#include "bug.h"
#include "compiler.h"
#include "db.h"
#include "hmm.h"
#include "job.h"
#include "logger.h"
#include "prod.h"
#include "scan.h"
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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct sqlite3 *sched = NULL;
char sched_filepath[PATH_SIZE] = {0};

enum sched_rc emerge_db(char const *filepath);
enum sched_rc is_empty(char const *filepath, bool *empty);

enum sched_rc sched_init(char const *filepath)
{
    strlcpy(sched_filepath, filepath, ARRAY_SIZE(sched_filepath));

    if (!xsql_is_thread_safe()) return efail("not thread safe");
    if (xsql_version() < XSQL_REQUIRED_VERSION) return efail("old sqlite3");

    enum sched_rc rc = xfile_touch(filepath);
    if (rc) return rc;

    bool empty = false;
    rc = is_empty(filepath, &empty);
    if (rc) return rc;

    if (empty && emerge_db(filepath)) return efail("emerge db");

    if (xsql_open(sched_filepath, &sched)) return EOPENSCHED;
    return stmt_init() ? (xsql_close(sched), EEXEC) : SCHED_OK;
}

enum sched_rc sched_cleanup(void)
{
    stmt_del();
    return xsql_close(sched);
}

enum sched_rc sched_wipe(void)
{
    enum sched_rc rc = xsql_begin_transaction(sched);
    if (rc)
    {
        rc = efail("begin wipe");
        goto cleanup;
    }

    rc = prod_delete();
    if (rc) goto cleanup;

    rc = seq_delete();
    if (rc) goto cleanup;

    rc = scan_delete();
    if (rc) goto cleanup;

    rc = db_delete();
    if (rc) goto cleanup;

    rc = hmm_delete();
    if (rc) goto cleanup;

    rc = job_delete();
    if (rc) goto cleanup;

    return xsql_end_transaction(sched) ? efail("end wipe") : SCHED_OK;

cleanup:
    return xsql_rollback_transaction(sched) ? efail("rollback wipe") : rc;
}

struct sqlite3 *sched_handle(void) { return sched; }

enum sched_rc emerge_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (xsql_open(filepath, &db)) return EOPENSCHED;

    if (xsql_exec(db, (char const *)schema, 0, 0))
        return (xsql_close(db), EEXEC);

    return xsql_close(db) ? efail("failed to close sched") : SCHED_OK;
}

static int is_empty_fn(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    unused(argc);
    unused(argv);
    unused(cols);
    return 0;
}

enum sched_rc is_empty(char const *filepath, bool *empty)
{
    struct sqlite3 *db = NULL;
    if (xsql_open(filepath, &db)) return EOPENSCHED;

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if (xsql_exec(db, sql, is_empty_fn, empty)) return (xsql_close(db), EEXEC);

    return xsql_close(db) ? efail("failed to close sched") : SCHED_OK;
}
