#include "prod.h"
#include "compiler.h"
#include "logger.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/sched.h"
#include "sqlite3/sqlite3.h"
#include "stmt.h"
#include "to.h"
#include "tok.h"
#include "xfile.h"
#include "xsql.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

enum
{
    COL_TYPE_INT,
    COL_TYPE_INT64,
    COL_TYPE_DOUBLE,
    COL_TYPE_TEXT
} col_type[9] = {COL_TYPE_INT64, COL_TYPE_INT64,  COL_TYPE_TEXT,
                 COL_TYPE_TEXT,  COL_TYPE_DOUBLE, COL_TYPE_DOUBLE,
                 COL_TYPE_TEXT,  COL_TYPE_TEXT,   COL_TYPE_TEXT};

static TOK_DECLARE(tok);
static struct xfile_tmp prod_file[MAX_NUM_THREADS] = {0};

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

enum sched_rc sched_prod_write_begin(struct sched_prod const *prod,
                                     unsigned thread_num)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file[thread_num].fp, fmt, prod->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, job_id)) efail("write prod");
    if (echo(Fd64, seq_id)) efail("write prod");

    if (echo(Fs, profile_name)) efail("write prod");
    if (echo(Fs, abc_name)) efail("write prod");

    /* Reference: https://stackoverflow.com/a/21162120 */
    if (echo(Fg, alt_loglik)) efail("write prod");
    if (echo(Fg, null_loglik)) efail("write prod");

    if (echo(Fs, profile_typeid)) efail("write prod");
    if (echo(Fs, version)) efail("write prod");

    return SCHED_OK;

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

enum sched_rc sched_prod_write_match(sched_prod_write_match_cb *cb,
                                     void const *match, unsigned thread_num)
{
    return cb(prod_file[thread_num].fp, match);
}

enum sched_rc sched_prod_write_match_sep(unsigned thread_num)
{
    if (fputc(';', prod_file[thread_num].fp) == EOF) return eio("fputc");
    return SCHED_OK;
}

enum sched_rc sched_prod_write_end(unsigned thread_num)
{
    if (fputc('\n', prod_file[thread_num].fp) == EOF) return eio("fputc");
    return SCHED_OK;
}

static enum sched_rc get_prod(struct sched_prod *prod)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(PROD_SELECT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, prod->id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("get db");

    int i = 0;
    prod->id = sqlite3_column_int64(st, i++);
    prod->job_id = sqlite3_column_int64(st, i++);
    prod->seq_id = sqlite3_column_int64(st, i++);

#define ecpy efail("copy txt")

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, profile_name))) return ecpy;
    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, abc_name))) return ecpy;

    prod->alt_loglik = sqlite3_column_double(st, i++);
    prod->null_loglik = sqlite3_column_double(st, i++);

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, profile_typeid))) return ecpy;
    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, version))) return ecpy;

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, match))) return ecpy;

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;

#undef ecpy
}

enum sched_rc prod_next(struct sched_prod *prod)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(PROD_SELECT_NEXT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, prod->id)) return efail("bind");
    if (xsql_bind_i64(st, 1, prod->job_id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) return efail("step");

    prod->id = sqlite3_column_int64(st, 0);
    if (xsql_step(st) != SCHED_END) return efail("step");

    return get_prod(prod);
}

enum sched_rc prod_delete(void)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(PROD_DELETE);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    return xsql_step(st) == SCHED_END ? SCHED_OK : efail("delete db");
}

#define CLEANUP(X)                                                             \
    do                                                                         \
    {                                                                          \
        rc = X;                                                                \
        goto cleanup;                                                          \
    } while (1)

static enum sched_rc expect_word(FILE *fp, char const *field)
{
    if (tok_next(&tok, fp)) return eparse("parse prods file");
    if (tok_id(&tok) != TOK_WORD) return eparse("parse prods file");
    if (strcmp(tok.value, field)) return eparse("parse prods file");
    return SCHED_OK;
}

static enum sched_rc parse_prod_file_header(FILE *fp)
{
    enum sched_rc rc = SCHED_OK;
    if ((rc = expect_word(fp, "job_id"))) return rc;
    if ((rc = expect_word(fp, "seq_id"))) return rc;
    if ((rc = expect_word(fp, "profile_name"))) return rc;
    if ((rc = expect_word(fp, "abc_name"))) return rc;
    if ((rc = expect_word(fp, "alt_loglik"))) return rc;
    if ((rc = expect_word(fp, "null_loglik"))) return rc;
    if ((rc = expect_word(fp, "profile_typeid"))) return rc;
    if ((rc = expect_word(fp, "version"))) return rc;
    if ((rc = expect_word(fp, "match"))) return rc;

    if (tok_next(&tok, fp)) return eparse("parse prods file");
    if (tok_id(&tok) != TOK_NL) return eparse("parse prods file");
    return rc;
}

enum sched_rc sched_prod_add_file(FILE *fp)
{
    struct sqlite3 *sched = sched_handle();
    enum sched_rc rc = SCHED_OK;
    if (xsql_begin_transaction(sched)) CLEANUP(efail("submit prod"));

    if ((rc = parse_prod_file_header(fp))) goto cleanup;

    do
    {
        struct xsql_stmt *stmt = stmt_get(PROD_INSERT);
        struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
        if (!st) CLEANUP(efail("submit prod"));
        if (tok_next(&tok, fp)) CLEANUP(eparse("parse prods file"));
        if (tok_id(&tok) == TOK_EOF) break;

        for (int i = 0; i < (int)ARRAY_SIZE(col_type); i++)
        {
            if (col_type[i] == COL_TYPE_INT64)
            {
                int64_t val = 0;
                if (!to_int64(tok_value(&tok), &val))
                    CLEANUP(eparse("parse prods file"));
                if (xsql_bind_i64(st, i, val)) CLEANUP(efail("submit prod"));
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok_value(&tok), &val))
                    CLEANUP(eparse("parse prods file"));
                if (xsql_bind_dbl(st, i, val)) CLEANUP(efail("submit prod"));
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                struct xsql_txt txt = {tok_size(&tok), tok_value(&tok)};
                if (xsql_bind_txt(st, i, txt)) CLEANUP(efail("submit prod"));
            }
            if (tok_next(&tok, fp)) CLEANUP(eparse("parse prods file"));
        }
        if (tok_id(&tok) != TOK_NL)
        {
            rc = eparse("expected newline");
            goto cleanup;
        }
        rc = xsql_step(st);
        if (rc == SCHED_EINVAL) CLEANUP(einval("constraint violation"));
        if (rc != SCHED_END) CLEANUP(efail("submit prod"));
    } while (true);

    if (xsql_end_transaction(sched)) CLEANUP(efail("submit prod"));
    return SCHED_OK;

cleanup:
    xsql_rollback_transaction(sched);
    return rc;
}

enum sched_rc sched_prod_get(struct sched_prod *prod)
{
#define ecpy efail("copy txt")

    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(PROD_SELECT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, prod->id)) return efail("bind");

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_NOTFOUND;
    if (rc != SCHED_OK) efail("get prod");

    prod->id = sqlite3_column_int64(st, 0);
    prod->job_id = sqlite3_column_int64(st, 1);
    prod->seq_id = sqlite3_column_int64(st, 2);

    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*prod, profile_name))) return ecpy;
    if (xsql_cpy_txt(st, 4, XSQL_TXT_OF(*prod, abc_name))) return ecpy;

    prod->alt_loglik = sqlite3_column_double(st, 5);
    prod->null_loglik = sqlite3_column_double(st, 6);

    if (xsql_cpy_txt(st, 7, XSQL_TXT_OF(*prod, profile_typeid))) return ecpy;
    if (xsql_cpy_txt(st, 8, XSQL_TXT_OF(*prod, version))) return ecpy;

    if (xsql_cpy_txt(st, 9, XSQL_TXT_OF(*prod, match))) return ecpy;

    if (xsql_step(st) != SCHED_END) return efail("step");
    return SCHED_OK;

#undef ecpy
}

enum sched_rc sched_prod_add(struct sched_prod *prod)
{
    struct sqlite3 *sched = sched_handle();
    struct xsql_stmt *stmt = stmt_get(PROD_INSERT);
    struct sqlite3_stmt *st = xsql_fresh_stmt(sched, stmt);
    if (!st) return efail("get fresh statement");

    if (xsql_bind_i64(st, 0, prod->job_id)) return efail("bind");
    if (xsql_bind_i64(st, 1, prod->seq_id)) return efail("bind");

    if (xsql_bind_str(st, 2, prod->profile_name)) return efail("bind");
    if (xsql_bind_str(st, 3, prod->abc_name)) return efail("bind");

    if (xsql_bind_dbl(st, 4, prod->alt_loglik)) return efail("bind");
    if (xsql_bind_dbl(st, 5, prod->null_loglik)) return efail("bind");

    if (xsql_bind_str(st, 6, prod->profile_typeid)) return efail("bind");
    if (xsql_bind_str(st, 7, prod->version)) return efail("bind");

    if (xsql_bind_str(st, 8, prod->match)) return efail("bind");

    if (xsql_step(st) != SCHED_END) return efail("step");
    prod->id = xsql_last_id(sched);
    return SCHED_OK;
}

#undef CLEANUP
