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

static int init_db(struct sched_db *db, char const *filepath)
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
    struct sched_db db = {0};
    int rc = init_db(&db, filepath);
    if (rc) return rc;

    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, db.xxh64))) return rc;
    if ((rc = xsql_bind_txt(stmt, 1, XSQL_TXT_OF(db, filepath)))) return rc;

    rc = xsql_step(stmt);
    if (rc != 2) return rc;
    *id = sqlite3_column_int64(stmt, 0);
    return xsql_end_step(stmt);
}

int db_has(char const *filepath)
{
    struct sched_db db = {0};
    int rc = init_db(&db, filepath);
    if (rc) return rc;
    return sched_db_get_by_xxh64(&db, db.xxh64);
}

static int select_db(struct sched_db *db, int64_t by_value,
                     enum stmt select_stmt)
{
    struct sqlite3_stmt *stmt = stmts[select_stmt];
    int rc = 0;
    if ((rc = xsql_reset(stmt))) return rc;

    if ((rc = xsql_bind_i64(stmt, 0, by_value))) return rc;

    rc = xsql_step(stmt);
    if (rc == 0) return SCHED_NOTFOUND;
    if (rc != 2) return SCHED_FAIL;

    db->id = sqlite3_column_int64(stmt, 0);
    db->xxh64 = sqlite3_column_int64(stmt, 1);
    if ((rc = xsql_cpy_txt(stmt, 2, XSQL_TXT_OF(*db, filepath)))) return rc;

    rc = xsql_end_step(stmt);
    if (rc) return SCHED_FAIL;
    return SCHED_FOUND;
}

int sched_db_get_by_id(struct sched_db *db, int64_t id)
{
    return select_db(db, id, SELECT_BY_ID);
}

int sched_db_get_by_xxh64(struct sched_db *db, int64_t xxh64)
{
    return select_db(db, xxh64, SELECT_BY_XXH64);
}

void db_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}
