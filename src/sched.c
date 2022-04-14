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
#include "sched_health.h"
#include "schema.h"
#include "seq.h"
#include "seq_queue.h"
#include "stmt.h"
#include "utc.h"
#include "xfile.h"
#include "xsql.h"
#include "xstrcpy.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

char sched_filepath[PATH_SIZE] = {0};

enum sched_rc emerge_sched(char const *filepath);
enum sched_rc is_empty(char const *filepath, bool *empty);

enum sched_rc sched_init(char const *filepath)
{
    if (!xstrcpy(sched_filepath, filepath, ARRAY_SIZE(sched_filepath)))
        return eio("filepath is too long");

    if (!xsql_is_thread_safe()) return efail("not thread safe");
    if (xsql_version() < XSQL_REQUIRED_VERSION) return efail("old sqlite3");

    enum sched_rc rc = xfile_touch(filepath);
    if (rc) return rc;

    bool empty = false;
    rc = is_empty(filepath, &empty);
    if (rc) return rc;

    if (empty && emerge_sched(filepath)) return efail("emerge sched");

    if (xsql_open(sched_filepath)) return EOPENSCHED;
    return stmt_init() ? (xsql_close(), EEXEC) : SCHED_OK;
}

enum sched_rc sched_health_check(struct sched_health *health)
{
    struct sched_db db = {0};
    struct sched_hmm hmm = {0};

    enum sched_rc rc = sched_db_get_all(health_check_db, &db, health);
    if (rc) return rc;

    return sched_hmm_get_all(health_check_hmm, &hmm, health);
}

enum sched_rc sched_cleanup(void)
{
    stmt_del();
    return xsql_close();
}

static void delete_db_file(struct sched_db *db, void *arg)
{
    (void)arg;
    if (remove(db->filename)) eio("failed to remove file");
}

static void delete_hmm_file(struct sched_hmm *hmm, void *arg)
{
    (void)arg;
    if (remove(hmm->filename)) eio("failed to remove file");
}

enum sched_rc sched_wipe(void)
{
    enum sched_rc rc = xsql_begin_transaction();
    if (rc)
    {
        rc = efail("begin wipe");
        goto cleanup;
    }

    rc = prod_wipe();
    if (rc) goto cleanup;

    rc = seq_wipe();
    if (rc) goto cleanup;

    rc = scan_wipe();
    if (rc) goto cleanup;

    struct sched_db db = {0};
    rc = sched_db_get_all(delete_db_file, &db, 0);
    if (rc) goto cleanup;

    rc = db_wipe();
    if (rc) goto cleanup;

    struct sched_hmm hmm = {0};
    rc = sched_hmm_get_all(delete_hmm_file, &hmm, 0);
    if (rc) goto cleanup;

    rc = hmm_wipe();
    if (rc) goto cleanup;

    rc = job_wipe();
    if (rc) goto cleanup;

    return xsql_end_transaction() ? efail("end wipe") : SCHED_OK;

cleanup:
    return xsql_rollback_transaction() ? efail("rollback wipe") : rc;
}

enum sched_rc emerge_sched(char const *filepath)
{
    if (xsql_open(filepath)) return EOPENSCHED;

    if (xsql_exec((char const *)schema, 0, 0)) return (xsql_close(), EEXEC);

    return xsql_close() ? efail("failed to close sched") : SCHED_OK;
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
    if (xsql_open(filepath)) return EOPENSCHED;

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if (xsql_exec(sql, is_empty_fn, empty)) return (xsql_close(), EEXEC);

    return xsql_close() ? efail("failed to close sched") : SCHED_OK;
}
