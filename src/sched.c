#include "dcp_sched/sched.h"
#include "compiler.h"
#include "db.h"
#include "job.h"
#include "prod.h"
#include "safe.h"
#include "schema.h"
#include "seq.h"
#include "seq_queue.h"
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
char sched_filepath[SCHED_PATH_SIZE] = {0};

#define MIN_SQLITE_VERSION 3035000

static_assert(SQLITE_VERSION_NUMBER >= MIN_SQLITE_VERSION,
              "We need RETURNING statement");

int emerge_db(char const *filepath);
int is_empty(char const *filepath, bool *empty);
int touch_db(char const *filepath);

int sched_setup(char const *filepath)
{
    safe_strcpy(sched_filepath, filepath, ARRAY_SIZE(sched_filepath));

    int thread_safe = sqlite3_threadsafe();
    if (thread_safe == 0) return SCHED_FAIL;
    if (sqlite3_libversion_number() < MIN_SQLITE_VERSION) return SCHED_FAIL;

    if (touch_db(filepath)) return SCHED_FAIL;

    bool empty = false;
    if (is_empty(filepath, &empty)) return SCHED_FAIL;

    if (empty && emerge_db(filepath)) return SCHED_FAIL;

    return SCHED_DONE;
}

int sched_open(void)
{
    if (xsql_open(sched_filepath, &sched)) goto cleanup;
    if (job_module_init()) goto cleanup;
    if (seq_module_init()) goto cleanup;
    if (prod_module_init()) goto cleanup;
    if (db_module_init()) goto cleanup;

    return SCHED_DONE;

cleanup:
    xsql_close(sched);
    return SCHED_FAIL;
}

int sched_close(void)
{
    db_module_del();
    prod_module_del();
    seq_module_del();
    job_module_del();
    return xsql_close(sched);
}

int sched_set_job_fail(int64_t job_id, char const *msg)
{
    return job_set_error(job_id, msg, utc_now());
}
int sched_set_job_done(int64_t job_id)
{
    return job_set_done(job_id, utc_now());
}

static int dbs_have_same_filepath(char const *filepath, bool *answer)
{
    int64_t xxh64 = 0;
    if (db_hash(filepath, &xxh64)) return SCHED_FAIL;
    struct db db = {0};
    if (db_get_by_xxh64(&db, xxh64)) return SCHED_FAIL;

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

    char resolved[PATH_MAX] = {0};
    char *ptr = realpath(filepath, resolved);
    if (!ptr) return SCHED_FAIL;

    if (rc == SCHED_NOTFOUND) return db_add(resolved, id);
    return SCHED_FAIL;
}

int sched_cpy_db_filepath(unsigned size, char *filepath, int64_t id)
{
    struct db db = {0};
    int code = db_get_by_id(&db, id);
    if (code == SCHED_NOTFOUND) return SCHED_NOTFOUND;
    if (code != SCHED_DONE) return SCHED_FAIL;
    safe_strcpy(filepath, db.filepath, size);
    return SCHED_DONE;
}

int sched_get_job(struct sched_job *job) { return job_get(job); }

int sched_begin_job_submission(struct sched_job *job)
{
    if (xsql_begin_transaction(sched)) return SCHED_FAIL;
    seq_queue_init();
    return SCHED_DONE;
}

void sched_add_seq(struct sched_job *job, char const *name, char const *data)
{
    seq_queue_add(job->id, name, data);
}

int sched_rollback_job_submission(struct sched_job *job)
{
    return xsql_rollback_transaction(sched);
}

int sched_end_job_submission(struct sched_job *job)
{
    if (job_submit(job)) return SCHED_FAIL;

    for (unsigned i = 0; i < seq_queue_size(); ++i)
    {
        struct sched_seq *seq = seq_queue_get(i);
        seq->job_id = job->id;
        if (seq_submit(seq)) return xsql_rollback_transaction(sched);
    }

    return xsql_end_transaction(sched);
}

int sched_begin_prod_submission(unsigned num_threads)
{
    assert(num_threads > 0);
    if (prod_begin_submission(num_threads)) return SCHED_FAIL;
    return SCHED_DONE;
}

int sched_end_prod_submission(void)
{
    if (prod_end_submission()) return SCHED_FAIL;
    return SCHED_DONE;
}

int sched_next_pending_job(struct sched_job *job)
{
    return job_next_pending(job);
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
