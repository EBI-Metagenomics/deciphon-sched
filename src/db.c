#include "db.h"
#include "compiler.h"
#include "dcp_sched/rc.h"
#include "logger.h"
#include "safe.h"
#include "sched.h"
#include "xfile.h"
#include "xsql.h"
#include <sqlite3.h>
#include <stdlib.h>

enum stmt
{
    INSERT,
    SELECT_BY_ID,
    SELECT_BY_XXH64
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO db\
            (\
                xxh64, filepath\
            )\
        VALUES\
            (\
                ?, ?\
            )\
        RETURNING id;\
",
    [SELECT_BY_ID] = "SELECT * FROM db WHERE id = ?;\
",
    [SELECT_BY_XXH64] = "SELECT * FROM db WHERE xxh64 = ?;\
"};
/* clang-format on */

extern struct sqlite3 *sched;
static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};

static int init_db(struct db *db, char const *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return error("failed to open file");

    int rc = xfile_hash(fp, (uint64_t *)&db->xxh64);
    if (rc) goto cleanup;

    safe_strcpy(db->filepath, filepath, DCP_PATH_SIZE);

cleanup:
    fclose(fp);
    return 0;
}

int db_module_init(void)
{
    int rc = 0;
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if ((rc = xsql_prepare(sched, queries[i], stmts + i))) return rc;
    }
    return 0;
}

int db_add(char const *filepath, int64_t *id)
{
    struct sqlite3_stmt *stmt = stmts[INSERT];
    struct db db = {0};
    if (init_db(&db, filepath)) return SCHED_FAIL;

    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, db.xxh64)) return SCHED_FAIL;
    if (xsql_bind_txt(stmt, 1, XSQL_TXT_OF(db, filepath))) return SCHED_FAIL;

    if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL;
    *id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

int db_has(char const *filepath)
{
    struct db db = {0};
    if (init_db(&db, filepath)) return SCHED_FAIL;
    return db_get_by_xxh64(&db, db.xxh64);
}

int db_hash(char const *filepath, int64_t *xxh64)
{
    struct db db = {0};
    if (init_db(&db, filepath)) return SCHED_FAIL;
    *xxh64 = db.xxh64;
    return SCHED_DONE;
}

static int select_db(struct db *db, int64_t by_value, enum stmt select_stmt)
{
    struct sqlite3_stmt *stmt = stmts[select_stmt];
    if (xsql_reset(stmt)) return SCHED_DONE;

    if (xsql_bind_i64(stmt, 0, by_value)) return SCHED_DONE;

    int rc = xsql_step(stmt);
    if (rc == SCHED_DONE) return SCHED_NOTFOUND;
    if (rc != SCHED_NEXT) return SCHED_FAIL;

    db->id = sqlite3_column_int64(stmt, 0);
    db->xxh64 = sqlite3_column_int64(stmt, 1);
    if (xsql_cpy_txt(stmt, 2, XSQL_TXT_OF(*db, filepath))) return SCHED_DONE;

    return xsql_end_step(stmt);
}

int db_get_by_id(struct db *db, int64_t id)
{
    return select_db(db, id, SELECT_BY_ID);
}

int db_get_by_xxh64(struct db *db, int64_t xxh64)
{
    return select_db(db, xxh64, SELECT_BY_XXH64);
}

void db_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
