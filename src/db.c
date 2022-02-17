#include "db.h"
#include "logger.h"
#include "safe.h"
#include "sched/db.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "stmt.h"
#include "xfile.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>

extern struct sqlite3 *sched;

static enum sched_rc init_db(struct sched_db *db, char const *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    enum sched_rc rc = xfile_hash(fp, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->filename, filename, ARRAY_SIZE_OF(*db, filename));

cleanup:
    fclose(fp);
    return rc;
}

static enum sched_rc select_db_i64(struct sched_db *db, int64_t by_value,
                                   enum stmt select_stmt)
{
    struct sqlite3_stmt *st = stmt[select_stmt];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, by_value)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_DONE) return efail("get db");

    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename)))
        return efail("copy txt");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_DONE;
}

static enum sched_rc select_db_str(struct sched_db *db, char const *by_value,
                                   enum stmt select_stmt)
{
    struct sqlite3_stmt *st = stmt[select_stmt];
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_str(st, 0, by_value)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_DONE) return efail("get db");

    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename)))
        return efail("copy txt");

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_DONE;
}

static enum sched_rc add_db(char const *filename, struct sched_db *db)
{
    struct sqlite3_stmt *st = stmt[DB_INSERT];

    enum sched_rc rc = init_db(db, filename);
    if (rc) return rc;

    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, db->xxh64)) return efail("bind");
    if (xsql_bind_str(st, 1, filename)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("add db");
    db->id = xsql_last_id(sched);
    return SCHED_DONE;
}

enum sched_rc sched_db_add(struct sched_db *db, char const *filename)
{
    struct sched_db tmp = {0};
    enum sched_rc rc = select_db_str(&tmp, filename, DB_SELECT_BY_FILENAME);

    if (rc == SCHED_DONE)
        return error(SCHED_EFAIL, "db with same filename already exist");

    if (rc == SCHED_NOTFOUND) return add_db(filename, db);

    return rc;
}

static enum sched_rc db_next(struct sched_db *db)
{
#define ecpy efail("copy txt")

    struct sqlite3_stmt *st = stmt[DB_SELECT_NEXT];
    int rc = SCHED_DONE;
    if (xsql_reset(st)) return efail("reset");

    if (xsql_bind_i64(st, 0, db->id)) return efail("bind");

    rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_DONE) return efail("step");

    db->id = sqlite3_column_int64(st, 0);
    db->xxh64 = sqlite3_column_int64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename))) return ecpy;
    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_DONE;

#undef ecpy
}

enum sched_rc sched_db_get_all(sched_db_set_cb cb, struct sched_db *db,
                               void *arg)
{
    enum sched_rc rc = SCHED_DONE;

    sched_db_init(db);
    while ((rc = db_next(db)) == SCHED_DONE)
    {
        cb(db, arg);
    }
    return rc == SCHED_NOTFOUND ? SCHED_DONE : rc;
}

enum sched_rc db_has(char const *filename, struct sched_db *db)
{
    enum sched_rc rc = init_db(db, filename);
    if (rc) return rc;
    return db_get_by_xxh64(db, db->xxh64);
}

enum sched_rc db_get_by_xxh64(struct sched_db *db, int64_t xxh64)
{
    return select_db_i64(db, xxh64, DB_SELECT_BY_XXH64);
}

enum sched_rc db_hash(char const *filename, int64_t *xxh64)
{
    struct sched_db db = {0};
    enum sched_rc rc = init_db(&db, filename);
    *xxh64 = db.xxh64;
    return rc;
}

void sched_db_init(struct sched_db *db)
{
    db->id = 0;
    db->xxh64 = 0;
    db->filename[0] = 0;
}

enum sched_rc sched_db_get(struct sched_db *db)
{
    if (db->id) return select_db_i64(db, db->id, DB_SELECT_BY_ID);
    if (db->xxh64) return select_db_i64(db, db->xxh64, DB_SELECT_BY_XXH64);
    return select_db_str(db, db->filename, DB_SELECT_BY_FILENAME);
}
