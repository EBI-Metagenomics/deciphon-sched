#include "prod.h"
#include "error.h"
#include "sched/prod.h"
#include "sched/rc.h"
#include "sched/scan.h"
#include "sched/seq.h"
#include "stmt.h"
#include "to.h"
#include "tok.h"
#include "xfile.h"
#include "xsql.h"
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

static void prod_init(struct sched_prod *prod)
{
    prod->id = 0;

    prod->scan_id = 0;
    prod->seq_id = 0;

    prod->profile_name[0] = 0;
    prod->abc_name[0] = 0;

    prod->alt_loglik = 0.;
    prod->null_loglik = 0.;

    prod->profile_typeid[0] = 0;
    prod->version[0] = 0;

    prod->match[0] = 0;
}

void sched_prod_init(struct sched_prod *prod, int64_t scan_id)
{
    prod_init(prod);
    prod->scan_id = scan_id;
}

enum sched_rc sched_prod_write_begin(struct sched_prod const *prod,
                                     unsigned thread_num)
{
#define TAB "\t"
#define echo(fmt, var) fprintf(prod_file[thread_num].fp, fmt, prod->var) < 0
#define Fd64 "%" PRId64 TAB
#define Fs "%s" TAB
#define Fg "%.17g" TAB

    if (echo(Fd64, scan_id)) EWRITEFILE;
    if (echo(Fd64, seq_id)) EWRITEFILE;

    if (echo(Fs, profile_name)) EWRITEFILE;
    if (echo(Fs, abc_name)) EWRITEFILE;

    /* Reference: https://stackoverflow.com/a/21162120 */
    if (echo(Fg, alt_loglik)) EWRITEFILE;
    if (echo(Fg, null_loglik)) EWRITEFILE;

    if (echo(Fs, profile_typeid)) EWRITEFILE;
    if (echo(Fs, version)) EWRITEFILE;

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

enum sched_rc sched_prod_write_match(sched_prod_write_match_func_t *fn,
                                     void const *match, unsigned thread_num)
{
    return fn(prod_file[thread_num].fp, match);
}

enum sched_rc sched_prod_write_match_sep(unsigned thread_num)
{
    if (fputc(';', prod_file[thread_num].fp) == EOF) return EWRITEFILE;
    return SCHED_OK;
}

enum sched_rc sched_prod_write_end(unsigned thread_num)
{
    if (fputc('\n', prod_file[thread_num].fp) == EOF) return EWRITEFILE;
    return SCHED_OK;
}

static enum sched_rc get_prod(struct sched_prod *prod)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_GET));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, prod->id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_PROD_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;

    int i = 0;
    prod->id = xsql_get_i64(st, i++);
    prod->scan_id = xsql_get_i64(st, i++);
    prod->seq_id = xsql_get_i64(st, i++);

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, profile_name))) return EGETTXT;
    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, abc_name))) return EGETTXT;

    prod->alt_loglik = xsql_get_dbl(st, i++);
    prod->null_loglik = xsql_get_dbl(st, i++);

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, profile_typeid)))
        return EGETTXT;
    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, version))) return EGETTXT;

    if (xsql_cpy_txt(st, i++, XSQL_TXT_OF(*prod, match))) return EGETTXT;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc prod_scan_next(struct sched_prod *prod)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_GET_SCAN_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, prod->id)) return EBIND;
    if (xsql_bind_i64(st, 1, prod->scan_id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_PROD_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;

    prod->id = xsql_get_i64(st, 0);
    if (xsql_step(st) != SCHED_END) return ESTEP;

    return get_prod(prod);
}

enum sched_rc prod_next(struct sched_prod *prod)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_GET_NEXT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, prod->id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_PROD_NOT_FOUND;
    if (rc != SCHED_OK) return ESTEP;

    prod->id = xsql_get_i64(st, 0);
    if (xsql_step(st) != SCHED_END) return ESTEP;

    return get_prod(prod);
}

enum sched_rc prod_wipe(void)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_DELETE));
    if (!st) return EFRESH;

    enum sched_rc rc = xsql_step(st);
    return rc == SCHED_END ? SCHED_OK : ESTEP;
}

#define CLEANUP(X)                                                             \
    do                                                                         \
    {                                                                          \
        rc = X;                                                                \
        goto cleanup;                                                          \
    } while (1)

static enum sched_rc expect_word(FILE *fp, char const *field)
{
    if (tok_next(&tok, fp)) return EPARSEFILE;
    if (tok_id(&tok) != TOK_WORD) return EPARSEFILE;
    if (strcmp(tok.value, field)) return EPARSEFILE;
    return SCHED_OK;
}

static enum sched_rc parse_prod_file_header(FILE *fp)
{
    enum sched_rc rc = SCHED_OK;
    if ((rc = expect_word(fp, "scan_id"))) return rc;
    if ((rc = expect_word(fp, "seq_id"))) return rc;
    if ((rc = expect_word(fp, "profile_name"))) return rc;
    if ((rc = expect_word(fp, "abc_name"))) return rc;
    if ((rc = expect_word(fp, "alt_loglik"))) return rc;
    if ((rc = expect_word(fp, "null_loglik"))) return rc;
    if ((rc = expect_word(fp, "profile_typeid"))) return rc;
    if ((rc = expect_word(fp, "version"))) return rc;
    if ((rc = expect_word(fp, "match"))) return rc;

    if (tok_next(&tok, fp)) return EPARSEFILE;
    if (tok_id(&tok) != TOK_NL) return EPARSEFILE;
    return rc;
}

static enum sched_rc scan_exists(int64_t scan_id)
{
    struct sched_scan scan = {0};
    return sched_scan_get_by_id(&scan, scan_id);
}

static enum sched_rc seq_exists(int64_t seq_id)
{
    struct sched_seq seq = {0};
    return sched_seq_get_by_id(&seq, seq_id);
}

enum sched_rc sched_prod_add_file(FILE *fp)
{
    enum sched_rc rc = SCHED_OK;
    if (xsql_begin_transaction()) CLEANUP(EBEGINSTMT);

    if ((rc = parse_prod_file_header(fp))) goto cleanup;

    do
    {
        struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_INSERT));
        if (!st) CLEANUP(error(SCHED_FAIL_GET_FRESH_STMT));
        if (tok_next(&tok, fp)) CLEANUP(EPARSEFILE);
        if (tok_id(&tok) == TOK_EOF) break;

        for (int i = 0; i < (int)ARRAY_SIZE(col_type); i++)
        {
            if (col_type[i] == COL_TYPE_INT64)
            {
                int64_t val = 0;
                if (!to_int64(tok_value(&tok), &val)) CLEANUP(EPARSEFILE);
                if (xsql_bind_i64(st, i, val)) CLEANUP(EBIND);
                if (i == 0)
                {
                    rc = scan_exists(val);
                    if (rc) goto cleanup;
                }
                else if (i == 1)
                {
                    rc = seq_exists(val);
                    if (rc) goto cleanup;
                }
            }
            else if (col_type[i] == COL_TYPE_DOUBLE)
            {
                double val = 0;
                if (!to_double(tok_value(&tok), &val)) CLEANUP(EPARSEFILE);
                if (xsql_bind_dbl(st, i, val)) CLEANUP(EBIND);
            }
            else if (col_type[i] == COL_TYPE_TEXT)
            {
                struct xsql_txt txt = {tok_size(&tok), tok_value(&tok)};
                if (xsql_bind_txt(st, i, txt)) CLEANUP(EBIND);
            }
            if (tok_next(&tok, fp)) CLEANUP(EPARSEFILE);
        }
        if (tok_id(&tok) != TOK_NL)
        {
            rc = EPARSEFILE;
            goto cleanup;
        }
        rc = xsql_step(st);
        if (rc != SCHED_END) CLEANUP(ESTEP);
    } while (true);

    if (xsql_end_transaction()) CLEANUP(EENDSTMT);
    return SCHED_OK;

cleanup:
    xsql_rollback_transaction();
    return rc;
}

enum sched_rc sched_prod_get_by_id(struct sched_prod *prod, int64_t id)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_GET));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, id)) return EBIND;

    enum sched_rc rc = xsql_step(st);
    if (rc == SCHED_END) return SCHED_PROD_NOT_FOUND;
    if (rc != SCHED_OK) ESTEP;

    prod->id = xsql_get_i64(st, 0);
    prod->scan_id = xsql_get_i64(st, 1);
    prod->seq_id = xsql_get_i64(st, 2);

    if (xsql_cpy_txt(st, 3, XSQL_TXT_OF(*prod, profile_name))) return EGETTXT;
    if (xsql_cpy_txt(st, 4, XSQL_TXT_OF(*prod, abc_name))) return EGETTXT;

    prod->alt_loglik = xsql_get_dbl(st, 5);
    prod->null_loglik = xsql_get_dbl(st, 6);

    if (xsql_cpy_txt(st, 7, XSQL_TXT_OF(*prod, profile_typeid))) return EGETTXT;
    if (xsql_cpy_txt(st, 8, XSQL_TXT_OF(*prod, version))) return EGETTXT;

    if (xsql_cpy_txt(st, 9, XSQL_TXT_OF(*prod, match))) return EGETTXT;

    return xsql_step(st) != SCHED_END ? ESTEP : SCHED_OK;
}

enum sched_rc sched_prod_add(struct sched_prod *prod)
{
    struct sqlite3_stmt *st = xsql_fresh_stmt(stmt_get(PROD_INSERT));
    if (!st) return EFRESH;

    if (xsql_bind_i64(st, 0, prod->scan_id)) return EBIND;
    if (xsql_bind_i64(st, 1, prod->seq_id)) return EBIND;

    if (xsql_bind_str(st, 2, prod->profile_name)) return EBIND;
    if (xsql_bind_str(st, 3, prod->abc_name)) return EBIND;

    if (xsql_bind_dbl(st, 4, prod->alt_loglik)) return EBIND;
    if (xsql_bind_dbl(st, 5, prod->null_loglik)) return EBIND;

    if (xsql_bind_str(st, 6, prod->profile_typeid)) return EBIND;
    if (xsql_bind_str(st, 7, prod->version)) return EBIND;

    if (xsql_bind_str(st, 8, prod->match)) return EBIND;

    if (xsql_step(st) != SCHED_END) return ESTEP;
    prod->id = xsql_last_id();
    return SCHED_OK;
}

enum sched_rc sched_prod_get_all(sched_prod_set_func_t fn,
                                 struct sched_prod *prod, void *arg)
{
    enum sched_rc rc = SCHED_OK;

    prod_init(prod);
    while ((rc = prod_next(prod)) == SCHED_OK)
        fn(prod, arg);
    return rc == SCHED_PROD_NOT_FOUND ? SCHED_OK : rc;
}

#undef CLEANUP
