#include "sched/prodset.h"
#include "error.h"
#include "fs.h"
#include "prod.h"
#include "sched/hmmer.h"
#include "sched/hmmer_filename.h"
#include "strlcat.h"
#include "strlcpy.h"
#include "xsql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLEANUP(X)                                                             \
    do                                                                         \
    {                                                                          \
        rc = X;                                                                \
        goto cleanup;                                                          \
    } while (1)

static int callb(struct sched_prod const *prod, void *arg)
{
    char *path = arg;
    struct sched_hmmer_filename x = {.scan_id = prod->scan_id,
                                     .seq_id = prod->seq_id};
    strcpy(x.profile_name, prod->profile_name);
    int n = (int)strlen(path);
    path[n] = '/';
    sched_hmmer_filename_setup(&x, path + n + 1);
    static struct sched_hmmer hmmer = {0};
    sched_hmmer_init(&hmmer, prod->id);

    int rc = 0;
    unsigned char *data = NULL;
    if (fs_exists(path))
    {
        long size = 0;
        rc = fs_readall(path, &size, &data);
        if (rc) goto cleanup;

        rc = sched_hmmer_add(&hmmer, (int)size, data);
    }

cleanup:
    free(data);
    path[n] = '\0';
    return rc;
}

enum sched_rc sched_prodset_add(char const *dir)
{
    static char filename[FILENAME_MAX] = {0};

    size_t n = sizeof filename;
    sched_strlcpy(filename, dir, n);
    sched_strlcat(filename, "/prod.tsv", n);

    FILE *fp = fopen(filename, "rb");
    if (!fp) return error(SCHED_FAIL_OPEN_FILE);

    enum sched_rc rc = SCHED_OK;
    if (xsql_begin_transaction()) CLEANUP(EBEGINSTMT);

    sched_strlcpy(filename, dir, n);
    if ((rc = sched_prod_add_transaction(fp, &callb, filename))) goto cleanup;

    if (xsql_end_transaction()) CLEANUP(EENDSTMT);

    fclose(fp);
    return rc;

cleanup:
    xsql_rollback_transaction();
    fclose(fp);
    return rc;
}

#undef CLEANUP
