#include "hmm.h"
#include "logger.h"
#include "sched/hmm.h"
#include "sched/rc.h"
#include "stmt.h"
#include "xfile.h"
#include "xsql.h"
#include "xstrcpy.h"
#include <stdlib.h>
#include <string.h>

static enum sched_rc select_hmm_i64(struct sched_hmm *hmm, int64_t by_value,
                                    enum stmt select_stmt)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(select_stmt));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get hmm");

    hmm->id = xsql_get_i64(st, 0);
    hmm->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*hmm, filename))) return ECPYTXT;
    hmm->job_id = xsql_get_i64(st, 3);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

static enum sched_rc select_hmm_str(struct sched_hmm *hmm, char const *by_value,
                                    enum stmt select_stmt)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(select_stmt));
    if (!st) return EFRESH;

    if (xsql_bind_str(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get hmm");

    hmm->id = xsql_get_i64(st, 0);
    hmm->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*hmm, filename))) return ECPYTXT;
    hmm->job_id = xsql_get_i64(st, 3);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

void sched_hmm_init(struct sched_hmm *hmm)
{
    hmm->id = 0;
    hmm->xxh3 = 0;
    hmm->filename[0] = 0;
    hmm->job_id = 0;
}

static enum sched_rc hash_setup(struct sched_hmm *hmm)
{
    FILE *fp = fopen(hmm->filename, "rb");
    if (!fp) return eio("fopen");

    enum sched_rc rc = xfile_hash(fp, &hmm->xxh3);

    fclose(fp);
    return rc;
}

static enum sched_rc check_filename(char const *filename)
{
    if (!xfile_is_name(filename)) return einval("invalid hmm filename");

    size_t len = strlen(filename);
    if (len < 5) return einval("filename is too short");
    if (strncmp(&filename[len - 4], ".hmm", 4))
        return einval("invalid extension");

    return len >= FILENAME_SIZE ? einval("filename is too long") : SCHED_OK;
}

enum sched_rc sched_hmm_set_file(struct sched_hmm *hmm, char const *filename)
{
    enum sched_rc rc = check_filename(filename);
    if (rc) return rc;
    strcpy(hmm->filename, filename);
    return hash_setup(hmm);
}

enum sched_rc sched_hmm_get_by_id(struct sched_hmm *hmm, int64_t id)
{
    return select_hmm_i64(hmm, id, HMM_GET_BY_ID);
}

enum sched_rc sched_hmm_get_by_job_id(struct sched_hmm *hmm, int64_t job_id)
{
    return select_hmm_i64(hmm, job_id, HMM_GET_BY_JOB_ID);
}

enum sched_rc sched_hmm_get_by_xxh3(struct sched_hmm *hmm, int64_t xxh3)
{
    return select_hmm_i64(hmm, xxh3, HMM_GET_BY_XXH3);
}

enum sched_rc sched_hmm_get_by_filename(struct sched_hmm *hmm,
                                        char const *filename)
{
    return select_hmm_str(hmm, filename, HMM_GET_BY_FILENAME);
}

static enum sched_rc hmm_next(struct sched_hmm *hmm)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMM_GET_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, hmm->id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return ESTEP;

    hmm->id = xsql_get_i64(st, 0);
    hmm->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*hmm, filename))) return ECPYTXT;
    hmm->job_id = xsql_get_i64(st, 3);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_hmm_get_all(sched_hmm_set_func_t fn, struct sched_hmm *hmm,
                                void *arg)
{
    enum sched_rc rc = SCHED_OK;

    sched_hmm_init(hmm);
    while ((rc = hmm_next(hmm)) == SCHED_OK)
        fn(hmm, arg);
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

enum sched_rc sched_hmm_remove(int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMM_DELETE_BY_ID));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

static enum sched_rc submit(struct sched_hmm *hmm)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMM_INSERT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, hmm->xxh3)) return EBIND;
    if (xsql_bind_str(st, 1, hmm->filename)) return EBIND;
    if (xsql_bind_i64(st, 2, hmm->job_id)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    hmm->id = xsql_last_id();
    return SCHED_OK;
}

static enum sched_rc has_hmm_by_xxh3(int64_t xxh3)
{
    struct sched_hmm tmp = {0};
    return sched_hmm_get_by_xxh3(&tmp, xxh3);
}

enum sched_rc hmm_submit(void *hmm, int64_t job_id)
{
    struct sched_hmm *h = hmm;
    if (!h->filename[0]) return einval("file has not been set");
    h->job_id = job_id;

    enum sched_rc rc = has_hmm_by_xxh3(h->xxh3);
    if (rc == SCHED_OK) return einval("hmm already exists");

    if (rc != SCHED_NOTFOUND) return rc;

    return rc == SCHED_NOTFOUND ? submit(h) : rc;
}

enum sched_rc hmm_delete(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(HMM_DELETE));
    if (!st) return EFRESH;

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete hmm");
}

void hmm_to_db_filename(char *filename)
{
    size_t len = strlen(filename);
    filename[len - 3] = 'd';
    filename[len - 2] = 'c';
    filename[len - 1] = 'p';
}
