#include "xfile.h"
#include "compiler.h"
#include "dcp_sched/rc.h"
#include "safe.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <xxhash.h>

#define BUFFSIZE (8 * 1024)

int xfile_mktemp(char *filepath)
{
    if (mkstemp(filepath) == -1) return SCHED_FAIL;
    return SCHED_DONE;
}

static_assert(same_type(XXH64_hash_t, uint64_t), "XXH64_hash_t is uint64_t");

int xfile_hash(FILE *restrict fp, uint64_t *hash)
{
    int rc = SCHED_FAIL;
    XXH64_state_t *const state = XXH64_createState();
    if (!state) goto cleanup;
    XXH64_reset(state, 0);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp)) goto cleanup;

        XXH64_update(state, buffer, n);
    }
    if (ferror(fp)) goto cleanup;

    rc = SCHED_DONE;
    *hash = XXH64_digest(state);

cleanup:
    XXH64_freeState(state);
    return rc;
}

int xfile_tmp_open(struct xfile_tmp *file)
{
    safe_strcpy(file->path, XFILE_PATH_TEMP_TEMPLATE,
                ARRAY_SIZE_OF(*file, path));
    file->fp = 0;
    if (xfile_mktemp(file->path)) return SCHED_FAIL;
    if (!(file->fp = fopen(file->path, "wb+"))) return SCHED_FAIL;
    return SCHED_DONE;
}

int xfile_tmp_rewind(struct xfile_tmp *file)
{
    if (fflush(file->fp)) return SCHED_FAIL;
    rewind(file->fp);
    return SCHED_DONE;
}

void xfile_tmp_del(struct xfile_tmp const *file)
{
    fclose(file->fp);
    remove(file->path);
}
