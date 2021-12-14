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
                job_id,         seq_id,      \
                profile_name,   abc_name,    \
                alt_loglik,     null_loglik, \
                profile_typeid, version,     \
                match                        \
            )\
        VALUES\
            (\
                ?, ?,\
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
static unsigned nthreads = 0;
static struct xfile_tmp prod_file[SCHED_MAX_NUM_THREADS] = {0};

void sched_prod_init(struct sched_prod *prod, int64_t job_id)
{
    prod->id = 0;

    prod->job_id = job_id;
    prod->seq_id = 0;

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

static void cleanup(void)
{
    for (unsigned i = 0; i < nthreads; ++i)
        xfile_tmp_del(prod_file + i);
    nthreads = 0;
}

int prod_begin_submission(unsigned num_threads)
{
    assert(num_threads <= SCHED_MAX_NUM_THREADS);
    for (nthreads = 0; nthreads < num_threads; ++nthreads)
    {
        if (xfile_tmp_open(prod_file + nthreads))
        {
            cleanup();
            return SCHED_FAIL;
        }
    }
    return SCHED_DONE;
}

int sched_prod_write_begin(struct sched_prod const *prod, unsigned thread_num)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file[thread_num].fp, fmt, prod->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, job_id)) return SCHED_FAIL;
    if (echo(Fd64, seq_id)) return SCHED_FAIL;

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

int sched_prod_write_match(sched_prod_write_match_cb *cb, void const *match,
                           unsigned thread_num)
{
    return cb(prod_file[thread_num].fp, match);
}

int sched_prod_write_match_sep(unsigned thread_num)
{
    if (fputc(';', prod_file[thread_num].fp) == EOF) return SCHED_FAIL;
    return SCHED_DONE;
}

int sched_prod_write_end(unsigned thread_num)
{
    if (fputc('\n', prod_file[thread_num].fp) == EOF) return SCHED_FAIL;
    return SCHED_DONE;
}

static int submit_prod_file(FILE *restrict fp);

int prod_end_submission(void)
{
    int rc = SCHED_FAIL;

    for (unsigned i = 0; i < nthreads; ++i)
    {
        if (xfile_tmp_rewind(prod_file + i)) goto cleanup;
        if (submit_prod_file(prod_file[i].fp)) goto cleanup;
    }
    rc = SCHED_DONE;

cleanup:
    cleanup();
    return rc;
}

static int get_prod(struct sched_prod *prod)
{
    struct sqlite3_stmt *stmt = stmts[SELECT];
    if (xsql_reset(stmt)) return SCHED_FAIL;

    if (xsql_bind_i64(stmt, 0, prod->id)) return SCHED_FAIL;

    if (xsql_step(stmt) != SCHED_NEXT) return SCHED_FAIL;

    int i = 0;
    prod->id = sqlite3_column_int64(stmt, i++);

    prod->job_id = sqlite3_column_int64(stmt, i++);
    prod->seq_id = sqlite3_column_int64(stmt, i++);

    if (xsql_cpy_txt(stmt, i++, XSQL_TXT_OF(*prod, profile_name)))
        return SCHED_FAIL;
    if (xsql_cpy_txt(stmt, i++, XSQL_TXT_OF(*prod, abc_name)))
        return SCHED_FAIL;

    prod->alt_loglik = sqlite3_column_double(stmt, i++);
    prod->null_loglik = sqlite3_column_double(stmt, i++);

    if (xsql_cpy_txt(stmt, i++, XSQL_TXT_OF(*prod, profile_typeid)))
        return SCHED_FAIL;
    if (xsql_cpy_txt(stmt, i++, XSQL_TXT_OF(*prod, version))) return SCHED_FAIL;

    if (xsql_cpy_txt(stmt, i++, XSQL_TXT_OF(*prod, match))) return SCHED_FAIL;

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
    if (xsql_begin_transaction(sched)) goto cleanup;

    struct sqlite3_stmt *stmt = stmts[INSERT];

    do
    {
        if (xsql_reset(stmt)) goto cleanup;
        if (tok_next(&tok, fp)) goto cleanup;
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
            if (tok_next(&tok, fp)) goto cleanup;
        }
        assert(tok_id(&tok) == TOK_NL);
        if (xsql_step(stmt) != SCHED_NEXT) goto cleanup;
        if (xsql_end_step(stmt)) goto cleanup;
    } while (true);

    return xsql_end_transaction(sched);

cleanup:
    xsql_rollback_transaction(sched);
    return SCHED_FAIL;
}
