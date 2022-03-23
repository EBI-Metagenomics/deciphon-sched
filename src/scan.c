#include "scan.h"
#include "bug.h"
#include "compiler.h"
#include "logger.h"
#include "prod.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/scan.h"
#include "sched/sched.h"
#include "seq.h"
#include "seq_queue.h"
#include "stmt.h"
#include "strlcpy.h"
#include "utc.h"
#include "xsql.h"
#include <stdlib.h>
#include <string.h>

void sched_scan_init(struct sched_scan *scan, int64_t db_id, bool multi_hits,
                     bool hmmer3_compat)
{
    scan->id = 0;
    scan->db_id = db_id;
    scan->multi_hits = multi_hits;
    scan->hmmer3_compat = hmmer3_compat;
    scan->job_id = 0;
    seq_queue_init();
}

enum sched_rc sched_scan_get_seqs(int64_t scan_id, sched_seq_set_func_t fn,
                                  struct sched_seq *seq, void *arg)
{
    struct sched_scan scan = {0};
    enum sched_rc rc = sched_scan_get_by_scan_id(&scan, scan_id);
    if (rc) return rc;

    seq->id = 0;
    seq->scan_id = scan_id;
    while ((rc = sched_seq_next(seq)) == SCHED_OK)
    {
        fn(seq, arg);
    }
    return rc == SCHED_END ? SCHED_OK : rc;
}

enum sched_rc sched_scan_get_prods(int64_t scan_id, sched_prod_set_func_t fn,
                                   struct sched_prod *prod, void *arg)
{
    struct sched_scan scan = {0};
    enum sched_rc rc = sched_scan_get_by_scan_id(&scan, scan_id);
    if (rc) return rc;

    sched_prod_init(prod, scan_id);
    while ((rc = prod_next(prod)) == SCHED_OK)
    {
        fn(prod, arg);
    }
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

static enum sched_rc get_scan(struct sched_scan *scan, enum stmt stmt,
                              int64_t id)
{
    struct sqlite3 *sched = sched_handle();
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt_get(stmt));
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get scan");

    scan->id = xsql_get_i64(st, 0);

    scan->db_id = xsql_get_i64(st, 1);
    scan->multi_hits = xsql_get_int(st, 2);
    scan->hmmer3_compat = xsql_get_int(st, 3);
    scan->job_id = xsql_get_i64(st, 4);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

enum sched_rc sched_scan_get_by_scan_id(struct sched_scan *scan,
                                        int64_t scan_id)
{
    return get_scan(scan, SCAN_GET_BY_SCAN_ID, scan_id);
}

enum sched_rc sched_scan_get_by_job_id(struct sched_scan *scan, int64_t job_id)
{
    return get_scan(scan, SCAN_GET_BY_JOB_ID, job_id);
}

static enum sched_rc submit(struct sched_scan *scan)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SCAN_INSERT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, scan->db_id)) return efail("bind");
    if (xsql_bind_i64(st, 1, scan->multi_hits)) return efail("bind");
    if (xsql_bind_i64(st, 2, scan->hmmer3_compat)) return efail("bind");
    if (xsql_bind_i64(st, 3, scan->job_id)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    scan->id = xsql_last_id(sched);
    return SCHED_OK;
}

void sched_scan_add_seq(struct sched_scan *scan, char const *name,
                        char const *data)
{
    seq_queue_add(scan->id, name, data);
}

enum sched_rc scan_submit(void *scan, int64_t job_id)
{
    struct sched_scan *s = scan;
    s->job_id = job_id;

    enum sched_rc rc = submit(s);
    if (rc) return rc;

    for (unsigned i = 0; i < seq_queue_size(); ++i)
    {
        struct sched_seq *seq = seq_queue_get(i);
        seq->scan_id = s->id;
        if ((rc = seq_submit(seq))) break;
    }

    seq_queue_init();
    return rc;
}

void scan_rollback(void) { seq_queue_init(); }

enum sched_rc scan_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(SCAN_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete db");
}
//
// enum sched_rc sched_scan_end_submission(struct sched_scan *scan)
// {
//     enum sched_rc rc = scan_submit(scan);
//     if (rc) return rc;
//
//     for (unsigned i = 0; i < seq_queue_size(); ++i)
//     {
//         struct sched_seq *seq = seq_queue_get(i);
//         seq->scan_id = scan->id;
//         if ((rc = seq_submit(seq))) break;
//     }
//
//     seq_queue_init();
//     return rc;
// }
