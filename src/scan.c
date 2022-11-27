#include "scan.h"
#include "error.h"
#include "prod.h"
#include "sched/db.h"
#include "sched/hmmer.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/scan.h"
#include "sched/seq.h"
#include "seq.h"
#include "seq_queue.h"
#include "stmt.h"
#include "utc.h"
#include "xsql.h"
#include <stdlib.h>
#include <string.h>

void scan_init(struct sched_scan *scan)
{
    scan->id = 0;
    scan->db_id = 0;
    scan->multi_hits = 0;
    scan->hmmer3_compat = 0;
    scan->job_id = 0;
    seq_queue_init();
}

void sched_scan_init(struct sched_scan *scan, int64_t db_id, bool multi_hits,
                     bool hmmer3_compat)
{
    scan_init(scan);
    scan->db_id = db_id;
    scan->multi_hits = multi_hits;
    scan->hmmer3_compat = hmmer3_compat;
}

enum sched_rc sched_scan_get_seqs(int64_t scan_id, sched_seq_set_func_t fn,
                                  struct sched_seq *seq, void *arg)
{
    struct sched_scan scan = {0};
    enum sched_rc rc = sched_scan_get_by_id(&scan, scan_id);
    if (rc) return rc;

    seq->id = 0;
    seq->scan_id = scan_id;
    while ((rc = sched_seq_scan_next(seq)) == SCHED_OK)
    {
        fn(seq, arg);
    }
    return rc == SCHED_SEQ_NOT_FOUND ? SCHED_OK : rc;
}

enum sched_rc sched_scan_get_prods(int64_t scan_id,
                                   void (*callb)(struct sched_prod *,
                                                 struct sched_hmmer *, void *),
                                   struct sched_prod *prod,
                                   struct sched_hmmer *hmmer, void *arg)
{
    struct sched_scan scan = {0};
    enum sched_rc rc = sched_scan_get_by_id(&scan, scan_id);
    if (rc) return rc;

    sched_prod_init(prod, scan_id);
    while ((rc = prod_scan_next(prod)) == SCHED_OK)
    {
        rc = sched_hmmer_get_by_prod_id(hmmer, prod->id);
        if (rc) return rc;
        (*callb)(prod, hmmer, arg);
    }
    return rc == SCHED_PROD_NOT_FOUND ? SCHED_OK : rc;
}

static enum sched_rc set_scan(struct sched_scan *scan, struct sqlite3_stmt *st)
{
    scan->id = xsql_get_i64(st, 0);

    scan->db_id = xsql_get_i64(st, 1);
    scan->multi_hits = xsql_get_int(st, 2);
    scan->hmmer3_compat = xsql_get_int(st, 3);
    scan->job_id = xsql_get_i64(st, 4);

    return SCHED_OK;
}

static enum sched_rc get_scan(struct sched_scan *scan, enum stmt stmt,
                              int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(stmt));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_SCAN_NOT_FOUND;
    if (rc != SCHED_OK) ESTEP;

    if ((rc = set_scan(scan, st))) return rc;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_scan_get_by_id(struct sched_scan *scan, int64_t scan_id)
{
    return get_scan(scan, SCAN_GET_BY_ID, scan_id);
}

enum sched_rc sched_scan_get_by_job_id(struct sched_scan *scan, int64_t job_id)
{
    return get_scan(scan, SCAN_GET_BY_JOB_ID, job_id);
}

static enum sched_rc submit(struct sched_scan *scan)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SCAN_INSERT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, scan->db_id)) return EBIND;
    if (xsql_bind_i64(st, 1, scan->multi_hits)) return EBIND;
    if (xsql_bind_i64(st, 2, scan->hmmer3_compat)) return EBIND;
    if (xsql_bind_i64(st, 3, scan->job_id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    scan->id = xsql_last_id();
    return SCHED_OK;
}

void sched_scan_add_seq(char const *name, char const *data)
{
    seq_queue_add(name, data);
}

static enum sched_rc db_exists(int64_t db_id)
{
    struct sched_db db = {0};
    return sched_db_get_by_id(&db, db_id);
}

enum sched_rc scan_submit(void *scan, int64_t job_id)
{
    struct sched_scan *s = scan;
    s->job_id = job_id;

    enum sched_rc rc = db_exists(s->db_id);
    if (rc) return rc;

    rc = submit(s);
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

enum sched_rc scan_wipe(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SCAN_DELETE));
    if (!st) return EFRESH;

    enum sched_rc rc = xsql_step(st);
    return rc == SCHED_END ? SCHED_OK : ESTEP;
}

static enum sched_rc scan_next(struct sched_scan *scan)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(SCAN_GET_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, scan->id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_SCAN_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;

    if ((rc = set_scan(scan, st))) return rc;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_scan_get_all(sched_scan_set_func_t fn,
                                 struct sched_scan *scan, void *arg)
{
    enum sched_rc rc = SCHED_OK;

    scan_init(scan);
    while ((rc = scan_next(scan)) == SCHED_OK)
        fn(scan, arg);
    return rc == SCHED_SCAN_NOT_FOUND ? SCHED_OK : rc;
}
