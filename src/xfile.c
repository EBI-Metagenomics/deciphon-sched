#include "xfile.h"
#include "logger.h"
#include "sched/limits.h"
#include "xxhash/xxhash.h"
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/syslimits.h>
#endif

#define BUFFSIZE (8 * 1024)

static_assert(same_type(XXH64_hash_t, uint64_t), "XXH64_hash_t is uint64_t");

enum sched_rc xfile_hash(FILE *restrict fp, int64_t *hash)
{
    int rc = SCHED_EFAIL;
    XXH3_state_t *state = XXH3_createState();
    if (!state)
    {
        rc = efail("failed to create state");
        goto cleanup;
    }
    XXH3_64bits_reset(state);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = eio("fread");
            goto cleanup;
        }

        XXH3_64bits_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = eio("fread");
        goto cleanup;
    }
    rc = SCHED_OK;

    union
    {
        int64_t const i;
        uint64_t const u;
    } const h = {.u = XXH3_64bits_digest(state)};
    *hash = h.i;

cleanup:
    XXH3_freeState(state);
    return rc;
}

static char *glibc_basename(const char *filename)
{
    char *p = strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
}

static void xfile_basename(char *filename, char const *path)
{
    char *p = glibc_basename(path);
    strlcpy(filename, p, FILENAME_SIZE);
}

bool xfile_is_name(char const *filename)
{
    char really_filename[FILENAME_SIZE] = {0};
    xfile_basename(really_filename, filename);
    return !strcmp(filename, really_filename);
}

bool xfile_exists(char const *filepath) { return access(filepath, F_OK) == 0; }

enum sched_rc xfile_touch(char const *filepath)
{
    if (xfile_exists(filepath)) return SCHED_OK;
    int fd = open(filepath, O_RDWR | O_CREAT,
                  S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP);
    if (fd == -1) return eio("open");
    if (close(fd) == -1) return eio("close");
    return SCHED_OK;
}
