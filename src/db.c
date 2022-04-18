#include "db.h"
#include "logger.h"
#include "sched/db.h"
#include "sched/hmm.h"
#include "sched/rc.h"
#include "stmt.h"
#include "xfile.h"
#include "xsql.h"
#include "xstrcpy.h"
#include <stdlib.h>
#include <string.h>

void sched_db_init(struct sched_db *db)
{
    db->id = 0;
    db->xxh3 = 0;
    db->filename[0] = 0;
    db->hmm_id = 0;
}

static enum sched_rc select_db_i64(struct sched_db *db, int64_t by_value,
                                   enum stmt select_stmt)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(select_stmt));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get db");

    db->id = xsql_get_i64(st, 0);
    db->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename))) return ECPYTXT;
    db->hmm_id = xsql_get_i64(st, 3);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_db_get_by_id(struct sched_db *db, int64_t id)
{
    return select_db_i64(db, id, DB_GET_BY_ID);
}

enum sched_rc sched_db_get_by_xxh3(struct sched_db *db, int64_t xxh3)
{
    return select_db_i64(db, xxh3, DB_GET_BY_XXH3);
}

static enum sched_rc select_db_str(struct sched_db *db, char const *by_value,
                                   enum stmt select_stmt)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(select_stmt));
    if (!st) return EFRESH;

    if (xsql_bind_str(st, 0, by_value)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get db");

    db->id = xsql_get_i64(st, 0);
    db->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename))) return ECPYTXT;
    db->hmm_id = xsql_get_i64(st, 3);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_db_get_by_filename(struct sched_db *db,
                                       char const *filename)
{
    return select_db_str(db, filename, DB_GET_BY_FILENAME);
}

static enum sched_rc db_next(struct sched_db *db)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(DB_GET_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, db->id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return ESTEP;

    db->id = xsql_get_i64(st, 0);
    db->xxh3 = xsql_get_i64(st, 1);
    if (xsql_cpy_txt(st, 2, XSQL_TXT_OF(*db, filename))) return ECPYTXT;
    db->hmm_id = xsql_get_i64(st, 3);

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_db_get_all(sched_db_set_func_t fn, struct sched_db *db,
                               void *arg)
{
    enum sched_rc rc = SCHED_OK;

    sched_db_init(db);
    while ((rc = db_next(db)) == SCHED_OK)
        fn(db, arg);
    return rc == SCHED_NOTFOUND ? SCHED_OK : rc;
}

static enum sched_rc init_db(struct sched_db *db, char const *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    enum sched_rc rc = xfile_hash(fp, &db->xxh3);
    if (rc) goto cleanup;

    XSTRCPY(db, filename, filename);

cleanup:
    fclose(fp);
    return rc;
}

static enum sched_rc add_db(char const *filename, struct sched_db *db)
{
    enum sched_rc rc = init_db(db, filename);
    if (rc) return rc;

    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(DB_INSERT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, db->xxh3)) return EBIND;
    if (xsql_bind_str(st, 1, filename)) return EBIND;
    if (xsql_bind_i64(st, 2, db->hmm_id)) return EBIND;

    rc = xsql_step(st);
    if (rc == SCHED_EINVAL) return einval("add db");
    if (rc != SCHED_END) return efail("add db");

    db->id = xsql_last_id();
    return SCHED_OK;
}

static enum sched_rc has_db_by_filename(char const *filename)
{
    struct sched_db tmp = {0};
    return select_db_str(&tmp, filename, DB_GET_BY_FILENAME);
}

static enum sched_rc check_filename(char const *filename)
{
    if (!xfile_is_name(filename)) return einval("invalid db filename");

    size_t len = strlen(filename);
    if (len < 5) return einval("filename is too short");
    if (strncmp(&filename[len - 4], ".dcp", 4))
        return einval("invalid extension");

    return len >= FILENAME_SIZE ? einval("filename is too long") : SCHED_OK;
}

enum sched_rc sched_db_add(struct sched_db *db, char const *filename)
{
    enum sched_rc rc = check_filename(filename);
    if (rc) return rc;

    char hmm_filename[FILENAME_SIZE] = {0};
    strcpy(hmm_filename, filename);
    sched_db_to_hmm_filename(hmm_filename);

    struct sched_hmm hmm = {0};
    rc = sched_hmm_get_by_filename(&hmm, hmm_filename);
    if (rc == SCHED_NOTFOUND) return einval("hmm not found");
    if (rc) return rc;

    rc = has_db_by_filename(filename);
    if (rc == SCHED_OK) return einval("database already exist");

    db->hmm_id = hmm.id;
    return rc == SCHED_NOTFOUND ? add_db(filename, db) : rc;
}

enum sched_rc sched_db_remove(int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(DB_DELETE_BY_ID));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc != SCHED_END) return error(rc, "remove db");
    return xsql_changes() == 0 ? SCHED_NOTFOUND : SCHED_OK;
}

void sched_db_to_hmm_filename(char *filename)
{
    size_t len = strlen(filename);
    filename[len - 3] = 'h';
    filename[len - 2] = 'm';
    filename[len - 1] = 'm';
}

enum sched_rc db_wipe(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(DB_DELETE));
    if (!st) return EFRESH;

    enum sched_rc rc = xsql_step(st);
    return rc == SCHED_END ? SCHED_OK : error(rc, "wipe db");
}
