#include "db.h"
#include "logger.h"
#include "sched/db.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "stmt.h"
#include "strlcpy.h"
#include "xfile.h"
#include "xsql.h"
#include <stdlib.h>
#include <string.h>

static enum sched_rc init_db(struct sched_db *db, char const *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    enum sched_rc rc = xfile_hash(fp, (uint64_t *)&db->xxh3_64);
    if (rc) goto cleanup;

    strlcpy(db->filename, filename, ARRAY_SIZE_OF(*db, filename));

cleanup:
    fclose(fp);
    return rc;
}

static enum sched_rc select_db_i64(struct sched_db *db, int64_t by_value,
                                   enum stmt select_stmt)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(select_stmt);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, by_value)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get db");

    db->id = xsql_get_i64(st, 0);
    db->xxh3_64 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename)))
        return efail("copy txt");
    db->hmm_id = xsql_get_i64(st, 3);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

static enum sched_rc select_db_str(struct sched_db *db, char const *by_value,
                                   enum stmt select_stmt)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(select_stmt);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_str(st, 0, by_value)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get db");

    db->id = xsql_get_i64(st, 0);
    db->xxh3_64 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename)))
        return efail("copy txt");
    db->hmm_id = xsql_get_i64(st, 3);

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;
}

static enum sched_rc add_db(char const *filename, struct sched_db *db)
{
    struct sqlite3 *sched = sched_handle();
    enum sched_rc rc = init_db(db, filename);
    if (rc) return rc;

    struct xsql_stmt *stmt = stmt_get(DB_INSERT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, db->xxh3_64)) return efail("bind");
    if (xsql_bind_str(st, 1, filename)) return efail("bind");
    if (xsql_bind_i64(st, 2, db->hmm_id)) return efail("bind");

    rc = xsql_step(st);
    if (rc == SCHED_EINVAL) return einval("add db");
    if (rc != SCHED_END) return efail("add db");

    db->id = xsql_last_id(sched);
    return SCHED_OK;
}

enum sched_rc sched_db_add(struct sched_db *db, char const *filename)
{
    char really_filename[FILENAME_SIZE] = {0};
    xfile_basename(really_filename, filename);
    if (strcmp(filename, really_filename)) return einval("invalid db filename");

    struct sched_db tmp = {0};
    enum sched_rc rc = select_db_str(&tmp, filename, DB_SELECT_BY_FILENAME);

    if (rc == SCHED_OK) return einval("db with same filename already exist");

    if (rc == SCHED_NOTFOUND) return add_db(filename, db);

    return rc;
}

static enum sched_rc db_next(struct sched_db *db)
{
#define ecpy efail("copy txt")

    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(DB_SELECT_NEXT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, db->id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("step");

    db->id = xsql_get_i64(st, 0);
    db->xxh3_64 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename))) return ecpy;
    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;

#undef ecpy
}

enum sched_rc sched_db_get_all(sched_db_set_cb cb, struct sched_db *db,
                               void *arg)
{
    enum sched_rc rc = SCHED_OK;

    sched_db_init(db);
    while ((rc = db_next(db)) == SCHED_OK)
        cb(db, arg);
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

enum sched_rc db_has(char const *filename, struct sched_db *db)
{
    enum sched_rc rc = init_db(db, filename);
    if (rc) return rc;
    return db_get_by_xxh64(db, db->xxh3_64);
}

enum sched_rc db_get_by_xxh64(struct sched_db *db, int64_t xxh3_64)
{
    return select_db_i64(db, xxh3_64, DB_SELECT_BY_XXH3_64);
}

enum sched_rc db_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(DB_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete db");
}

enum sched_rc db_hash(char const *filename, int64_t *xxh3_64)
{
    struct sched_db db = {0};
    enum sched_rc rc = init_db(&db, filename);
    *xxh3_64 = db.xxh3_64;
    return rc;
}

void sched_db_init(struct sched_db *db)
{
    db->id = 0;
    db->xxh3_64 = 0;
    db->filename[0] = 0;
    db->hmm_id = 0;
}

enum sched_rc sched_db_get(struct sched_db *db)
{
    if (db->id) return select_db_i64(db, db->id, DB_SELECT_BY_ID);
    if (db->xxh3_64)
        return select_db_i64(db, db->xxh3_64, DB_SELECT_BY_XXH3_64);
    return select_db_str(db, db->filename, DB_SELECT_BY_FILENAME);
}
