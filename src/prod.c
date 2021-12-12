#include "prod.h"
#include "compiler.h"
#include "dcp_sched/prod.h"
#include "dcp_sched/rc.h"
#include "dcp_sched/sched.h"
#include "safe.h"
#include "sched.h"
#include "to.h"
#include "tok.h"
#include "xfile.h"
#include "xsql.h"
#include <assert.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdlib.h>

enum
{
    INSERT,
    SELECT,
    SELECT_NEXT
};

/* clang-format off */
static char const *const queries[] = {
    [INSERT] = \
"\
        INSERT INTO prod\
            (\
                job_id,         seq_id,      match_id,\
                profile_name,   abc_name,             \
                alt_loglik,     null_loglik,          \
                profile_typeid, version,              \
                match_data                            \
            )\
        VALUES\
            (\
                ?, ?, ?, \
                ?, ?,\
                ?, ?,\
                ?, ?,\
                ?\
            ) RETURNING id;\
",
    [SELECT] = "SELECT * FROM prod WHERE id = ?;\
",
    [SELECT_NEXT] = \
"\
        SELECT\
            id FROM prod\
        WHERE\
            id > ? AND job_id = ? ORDER BY id ASC LIMIT 1;\
"};
/* clang-format on */

enum
{
    COL_TYPE_INT,
    COL_TYPE_INT64,
    COL_TYPE_DOUBLE,
    COL_TYPE_TEXT
} col_type[12] = {COL_TYPE_INT64,  COL_TYPE_INT64, COL_TYPE_INT64,
                  COL_TYPE_TEXT,   COL_TYPE_TEXT,  COL_TYPE_DOUBLE,
                  COL_TYPE_DOUBLE, COL_TYPE_TEXT,  COL_TYPE_TEXT,
                  COL_TYPE_TEXT};

extern struct sqlite3 *sched;
static struct sqlite3_stmt *stmts[ARRAY_SIZE(queries)] = {0};
static TOK_DECLARE(tok);
struct xfile_tmp prod_file = {0};

void sched_prod_init(struct sched_prod *prod, int64_t job_id)
{
    prod->id = 0;

    prod->job_id = job_id;
    prod->seq_id = 0;
    prod->match_id = 0;

    prod->profile_name[0] = 0;
    prod->abc_name[0] = 0;

    prod->alt_loglik = 0.;
    prod->null_loglik = 0.;

    prod->profile_typeid[0] = 0;
    prod->version[0] = 0;

    prod->match[0] = 0;
}

int prod_module_init(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        if (xsql_prepare(sched, queries[i], stmts + i)) return SCHED_FAIL;
    }
    return 0;
}

int prod_begin_submission(void)
{
    if (xfile_tmp_open(&prod_file)) return SCHED_FAIL;
    return SCHED_DONE;
}

int sched_prod_write_begin(struct sched_prod const *prod)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file.fp, fmt, prod->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, job_id)) return SCHED_FAIL;
    if (echo(Fd64, seq_id)) return SCHED_FAIL;
    if (echo(Fd64, match_id)) return SCHED_FAIL;

    if (echo(Fs, profile_name)) return SCHED_FAIL;
    if (echo(Fs, abc_name)) return SCHED_FAIL;

    /* Reference: https://stackoverflow.com/a/21162120 */
    if (echo(Fg, alt_loglik)) return SCHED_FAIL;
    if (echo(Fg, null_loglik)) return SCHED_FAIL;

    if (echo(Fs, profile_typeid)) return SCHED_FAIL;
    if (echo(Fs, version)) return SCHED_FAIL;

    return SCHED_DONE;

#undef Fg
#undef Fs
#undef Fd64
#undef echo
#undef TAB
}

/* Output example
 *             ___________________________
 *             |   match0   |   match1   |
 *             ---------------------------
 * Output----->| CG,M1,CGA,K;CG,M4,CGA,K |
 *             ---|-|---|--|--------------
 * -----------   /  |   |  \    ---------------
 * | matched |__/   |   |   \___| most likely |
 * | letters |      |   |       | amino acid  |
 * -----------      |   |       ---------------
 *      -------------   ---------------
 *      | hmm state |   | most likely |
 *      -------------   | codon       |
 *                      ---------------
 */

int sched_prod_write_match(sched_prod_write_match_cb *cb, void const *match)
{
    return cb(prod_file.fp, match);
}

int sched_prod_write_match_sep(void)
{
    if (fputc(';', prod_file.fp) == EOF) return SCHED_FAIL;
    return SCHED_DONE;
}

int sched_prod_write_end(void)
{
    if (fputc('\n', prod_file.fp) == EOF) return SCHED_FAIL;
    return SCHED_DONE;
}

static int submit_prod_file(FILE *restrict fp);

int prod_end_submission(void)
{
    int rc = SCHED_FAIL;
    if (xfile_tmp_rewind(&prod_file)) goto cleanup;
    if (submit_prod_file(prod_file.fp)) goto cleanup;
    rc = SCHED_DONE;

cleanup:
    xfile_tmp_del(&prod_file);
    return rc;
}

/* int sched_prod_add(void) */
/* { */
/*     struct sqlite3_stmt *stmt = stmts[INSERT]; */
/*     if (xsql_reset(stmt)) return SCHED_FAIL; */
/*  */
/*     if (xsql_bind_i64(stmt, 0, prod.job_id)) return SCHED_FAIL; */
/*     if (xsql_bind_i64(stmt, 1, prod.seq_id)) return SCHED_FAIL; */
/*     if (xsql_bind_i64(stmt, 2, prod.match_id)) return SCHED_FAIL; */
/*  */
/*     if (xsql_bind_txt(stmt, 3, XSQL_TXT_OF(prod, profile_name))) */
/*         return SCHED_FAIL; */
/*     if (xsql_bind_txt(stmt, 4, XSQL_TXT_OF(prod, abc_name))) return
 * SCHED_FAIL; */
/*  */
/*     if (xsql_bind_dbl(stmt, 5, prod.alt_loglik)) return SCHED_FAIL; */
/*     if (xsql_bind_dbl(stmt, 6, prod.null_loglik)) return SCHED_FAIL; */
/*  */
/*     if (xsql_bind_txt(stmt, 7, XSQL_TXT_OF(prod, profile_typeid))) */
/*         return SCHED_FAIL; */
/*     if (xsql_bind_txt(stmt, 8, XSQL_TXT_OF(prod, version))) return
 * SCHED_FAIL; */
/*  */
/*     if (xsql_bind_txt(stmt, 9, XSQL_TXT_OF(prod, match))) return SCHED_FAIL;
 */
/*  */
/*     if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL; */
/*     prod.id = sqlite3_column_int64(stmt, 0); */
/*     return xsql_end_step(stmt); */
/* } */

static int get_prod(struct sched_prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, prod->id)) return SCHED_FAIL;

    if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL;

    prod->id = sqlite3_column_int64(stmt, 0);

    prod->job_id = sqlite3_column_int64(stmt, 1);
    prod->seq_id = sqlite3_column_int64(stmt, 2);
    prod->match_id = sqlite3_column_int64(stmt, 3);

    if (xsql_cpy_txt(stmt, 4, XSQL_TXT_OF(*prod, profile_name)))
        return SCHED_FAIL;
    if (xsql_cpy_txt(stmt, 5, XSQL_TXT_OF(*prod, abc_name))) return SCHED_FAIL;

    prod->alt_loglik = sqlite3_column_double(stmt, 6);
    prod->null_loglik = sqlite3_column_double(stmt, 7);

    if (xsql_cpy_txt(stmt, 8, XSQL_TXT_OF(*prod, profile_typeid)))
        return SCHED_FAIL;
    if (xsql_cpy_txt(stmt, 9, XSQL_TXT_OF(*prod, version))) return SCHED_FAIL;

    if (xsql_cpy_txt(stmt, 10, XSQL_TXT_OF(*prod, match))) return SCHED_FAIL;

    return xsql_end_step(stmt);
}

int sched_prod_next(struct sched_prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[SELECT_NEXT];
    int rc = SCHED_DONE;
    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, prod->id)) return SCHED_FAIL;
    if (xsql_bind_i64(stmt, 1, prod->job_id)) return SCHED_FAIL;

    rc = xsql_step(stmt);
    if (rc == SCHED_DONE) return SCHED_DONE;
    if (rc != SCHED_NEXT) return SCHED_FAIL;

    prod->id = sqlite3_column_int64(stmt, 0);
    if (xsql_end_step(stmt)) return SCHED_FAIL;

    if (get_prod(prod)) return SCHED_FAIL;
    return SCHED_NEXT;
}

void prod_module_del(void)
{
    for (unsigned i = 0; i < ARRAY_SIZE(stmts); ++i)
        sqlite3_finalize(stmts[i]);
}

static int submit_prod_file(FILE *restrict fp)
{
    int rc = SCHED_FAIL;
    if (xsql_begin_transaction(sched)) goto cleanup;

    struct sqlite3_stmt *stmt = stmts[INSERT];

    do
    {
        if (xsql_reset(stmt)) goto cleanup;
        rc = tok_next(&tok, fp);
        if (rc) return rc;
        if (tok_id(&tok) == TOK_EOF) break;

        for (int i = 0; i < 10; i++)
        {
            if (col_type[i] == COL_TYPE_INT64)
            {
                int64_t val = 0;
                if (!to_int64(tok_value(&tok), &val)) goto cleanup;
                if (xsql_bind_i64(stmt, i, val)) goto cleanup;
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok_value(&tok), &val)) goto cleanup;
                if (xsql_bind_dbl(stmt, i, val)) goto cleanup;
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                struct xsql_txt txt = {tok_size(&tok), tok_value(&tok)};
                if (xsql_bind_txt(stmt, i, txt)) goto cleanup;
            }
            rc = tok_next(&tok, fp);
        }
        assert(tok_id(&tok) == TOK_NL);
        if (xsql_step(stmt) != SCHED_NEXT) goto cleanup;
        if (xsql_end_step(stmt)) goto cleanup;
    } while (true);

cleanup:
    if (rc) return xsql_rollback_transaction(sched);
    return xsql_end_transaction(sched);
}
