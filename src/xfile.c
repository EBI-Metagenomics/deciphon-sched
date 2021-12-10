#include "xfile.h"
#include "compiler.h"
#include "logger.h"
#include "safe.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <xxhash.h>

#define BUFFSIZE (8 * 1024)

int xfile_mktemp(char *filepath)
{
    if (mkstemp(filepath) == -1) return error("mkstemp failed");
    return 0;
}

static_assert(same_type(XXH64_hash_t, uint64_t), "XXH64_hash_t is uint64_t");

int xfile_hash(FILE *restrict fp, uint64_t *hash)
{
    int rc = 0;
    XXH64_state_t *const state = XXH64_createState();
    if (!state)
    {
        rc = error("not enough memory for hashing");
        goto cleanup;
    }
    XXH64_reset(state, 0);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = error("failed to read file");
            goto cleanup;
        }

        XXH64_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = error("failed to read file");
        goto cleanup;
    }

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
    int rc = xfile_mktemp(file->path);
    if (rc) return rc;
    if (!(file->fp = fopen(file->path, "wb+")))
        rc = error("failed to open prod file");
    return rc;
}

int xfile_tmp_rewind(struct xfile_tmp *file)
{
    if (fflush(file->fp)) return error("failed to flush file");
    rewind(file->fp);
    return 0;
}

void xfile_tmp_del(struct xfile_tmp const *file)
{
    fclose(file->fp);
    remove(file->path);
}
