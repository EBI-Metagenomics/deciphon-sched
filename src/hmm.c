#include "hmm.h"
#include "logger.h"
#include "sched/hmm.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "stmt.h"
#include "strlcpy.h"
#include "xfile.h"
#include "xsql.h"
#include <stdlib.h>
#include <string.h>

static enum sched_rc init_hmm(struct sched_hmm *hmm, char const *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    enum sched_rc rc = xfile_hash(fp, (uint64_t *)&hmm->xxh3);
    if (rc) goto cleanup;

    strlcpy(hmm->filename, filename, ARRAY_SIZE_OF(*hmm, filename));

cleanup:
    fclose(fp);
    return rc;
}

static enum sched_rc select_hmm_i64(struct sched_hmm *hmm, int64_t by_value,
                                    enum stmt select_stmt)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(select_stmt);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get hmm");

    hmm->id = xsql_get_i64(st, 0);
    hmm->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*hmm, filename))) return ECPYTXT;
    hmm->job_id = xsql_get_i64(st, 3);

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

static enum sched_rc select_hmm_str(struct sched_hmm *hmm, char const *by_value,
                                    enum stmt select_stmt)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(select_stmt);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    if (xsql_bind_str(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get hmm");

    hmm->id = xsql_get_i64(st, 0);
    hmm->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*hmm, filename))) return ECPYTXT;
    hmm->job_id = xsql_get_i64(st, 3);

    if (xsql_step(st) != SCHED_END) return ESTEP;
    return SCHED_OK;
}

static enum sched_rc add_hmm(char const *filename, struct sched_hmm *hmm)
{
    struct sqlite3 *sched = sched_handle();
    enum sched_rc rc = init_hmm(hmm, filename);
    if (rc) return rc;

    struct xsql_stmt *stmt = stmt_get(HMM_INSERT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, hmm->xxh3)) return EBIND;
    if (xsql_bind_str(st, 1, filename)) return EBIND;
    if (xsql_bind_i64(st, 2, hmm->job_id)) return EBIND;

    rc = xsql_step(st);
    if (rc == SCHED_EINVAL) return einval("add hmm");
    if (rc != SCHED_END) return efail("add hmm");

    hmm->id = xsql_last_id(sched);
    return SCHED_OK;
}

enum sched_rc hmm_has(char const *filename, struct sched_hmm *hmm)
{
    enum sched_rc rc = init_hmm(hmm, filename);
    if (rc) return rc;
    return hmm_get_by_xxh3(hmm, hmm->xxh3);
}

enum sched_rc hmm_get_by_xxh3(struct sched_hmm *hmm, int64_t xxh3)
{
    return select_hmm_i64(hmm, xxh3, HMM_SELECT_BY_XXH3);
}

enum sched_rc hmm_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(HMM_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return EFRESH;

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete hmm");
}

enum sched_rc hmm_hash(char const *filename, int64_t *xxh3)
{
    struct sched_hmm hmm = {0};
    enum sched_rc rc = init_hmm(&hmm, filename);
    *xxh3 = hmm.xxh3;
    return rc;
}

void sched_hmm_init(struct sched_hmm *hmm)
{
    hmm->id = 0;
    hmm->xxh3 = 0;
    hmm->filename[0] = 0;
    hmm->job_id = 0;
}

enum sched_rc sched_hmm_get_by_id(struct sched_hmm *hmm, int64_t id)
{
    return select_hmm_i64(hmm, id, HMM_SELECT_BY_ID);
}

enum sched_rc sched_hmm_get_by_xxh3(struct sched_hmm *hmm, int64_t xxh3)
{
    return select_hmm_i64(hmm, xxh3, HMM_SELECT_BY_XXH3);
}

enum sched_rc sched_hmm_get_by_filename(struct sched_hmm *hmm,
                                        char const *filename)
{
    return select_hmm_str(hmm, filename, HMM_SELECT_BY_FILENAME);
}

enum sched_rc sched_hmm_add(struct sched_hmm *hmm, char const *filename)
{
    char really_filename[FILENAME_SIZE] = {0};
    xfile_basename(really_filename, filename);
    if (strcmp(filename, really_filename))
        return einval("invalid hmm filename");

    struct sched_hmm tmp = {0};
    enum sched_rc rc = select_hmm_str(&tmp, filename, HMM_SELECT_BY_FILENAME);

    if (rc == SCHED_OK) return einval("hmm with same filename already exist");

    if (rc == SCHED_NOTFOUND) return add_hmm(filename, hmm);

    return rc;
}
