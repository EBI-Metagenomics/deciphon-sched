#include "dcp_sched/sched.h"
#include "compiler.h"
#include "db.h"
#include "job.h"
#include "logger.h"
#include "prod.h"
#include "safe.h"
#include "schema.h"
#include "seq.h"
#include "seq_queue.h"
#include "sqldiff.h"
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
char sched_filepath[DCP_PATH_SIZE] = {0};
extern struct job job;

static_assert(SQLITE_VERSION_NUMBER >= 3035000, "We need RETURNING statement");

int check_integrity(char const *filepath, bool *ok);
int create_ground_truth_db(char *filepath);
int emerge_db(char const *filepath);
int is_empty(char const *filepath, bool *empty);
int submit_job(struct sqlite3_stmt *, struct job *, int64_t db_id,
               int64_t *job_id);
int touch_db(char const *filepath);

int sched_setup(char const *filepath)
{
    safe_strcpy(sched_filepath, filepath, ARRAY_SIZE(sched_filepath));
    int thread_safe = sqlite3_threadsafe();
    if (thread_safe == 0)
        return error("the provided sqlite3 is not thread-safe");

    int rc = touch_db(filepath);
    if (rc) return rc;

    bool empty = false;
    if ((rc = is_empty(filepath, &empty))) return rc;

    if (empty && (rc = emerge_db(filepath))) return rc;

    bool ok = false;
    if ((rc = check_integrity(filepath, &ok))) return rc;
    if (!ok) return error("damaged sched database");

    return rc;
}

int sched_open(void)
{
    int rc = 0;

    if ((rc = xsql_open(sched_filepath, &sched))) goto cleanup;
    if ((rc = job_module_init())) goto cleanup;
    if ((rc = seq_module_init())) goto cleanup;
    if ((rc = prod_module_init())) goto cleanup;
    if ((rc = db_module_init())) goto cleanup;

    return rc;

cleanup:
    xsql_close(sched);
    return rc;
}

int sched_close(void)
{
    db_module_del();
    prod_module_del();
    seq_module_del();
    job_module_del();
    return xsql_close(sched);
}

static int dbs_have_same_filepath(char const *filepath, bool *answer)
{
    int rc = SCHED_DONE;

    int64_t xxh64 = 0;
    if ((rc = db_hash(filepath, &xxh64))) return rc;
    struct db db = {0};
    if ((rc = db_get_by_xxh64(&db, xxh64))) return rc;

    char resolved[PATH_MAX] = {0};
    char *ptr = realpath(filepath, resolved);
    if (!ptr) return SCHED_FAIL;

    *answer = strcmp(db.filepath, resolved) == 0;
    return SCHED_DONE;
}

int sched_add_db(char const *filepath, int64_t *id)
{
    int rc = db_has(filepath);
    if (rc == SCHED_DONE)
    {
        bool answer = false;
        if ((rc = dbs_have_same_filepath(filepath, &answer))) return rc;
        if (answer) return SCHED_DONE;
        return SCHED_FAIL;
    }
    if (rc == SCHED_NOTFOUND) return db_add(filepath, id);
    return SCHED_FAIL;
}

int sched_begin_job_submission(int64_t db_id, bool multi_hits,
                               bool hmmer3_compat)
{
    if (xsql_begin_transaction(sched)) return SCHED_FAIL;
    job_init(db_id, multi_hits, hmmer3_compat);
    seq_queue_init();
    return SCHED_DONE;
}

void sched_add_seq(char const *name, char const *data)
{
    seq_queue_add(job.id, name, data);
}

int sched_end_job_submission(void)
{
    if (job_submit()) return SCHED_FAIL;

    for (unsigned i = 0; i < seq_queue_size(); ++i)
    {
        struct seq *seq = seq_queue_get(i);
        if (seq_submit(seq)) return xsql_rollback_transaction(sched);
    }

    return xsql_end_transaction(sched);
}

int check_integrity(char const *filepath, bool *ok)
{
    char tmp[] = XFILE_PATH_TEMP_TEMPLATE;
    int rc = 0;

    if ((rc = create_ground_truth_db(tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp, ok))) goto cleanup;

cleanup:
    remove(tmp);
    return rc;
}

int create_ground_truth_db(char *filepath)
{
    int rc = 0;
    if ((rc = xfile_mktemp(filepath))) return rc;
    if ((rc = touch_db(filepath))) return rc;
    if ((rc = emerge_db(filepath))) return rc;
    return rc;
}

int emerge_db(char const *filepath)
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

int is_empty(char const *filepath, bool *empty)
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

int touch_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (xsql_open(filepath, &db)) goto cleanup;
    return xsql_close(db);

cleanup:
    xsql_close(sched);
    return SCHED_FAIL;
}
