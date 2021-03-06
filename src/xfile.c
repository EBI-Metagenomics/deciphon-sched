#include "xfile.h"
#include "c_toolbelt/c_toolbelt.h"
#include "error.h"
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
    int rc = SCHED_OK;
    XXH3_state_t *state = XXH3_createState();
    if (!state)
    {
        rc = error(SCHED_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    XXH3_64bits_reset(state);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = error(SCHED_FAIL_READ_FILE);
            goto cleanup;
        }

        XXH3_64bits_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = error(SCHED_FAIL_READ_FILE);
        goto cleanup;
    }

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
    ctb_strlcpy(filename, p, SCHED_FILENAME_SIZE);
}

bool xfile_is_name(char const *filename)
{
    char really_filename[SCHED_FILENAME_SIZE] = {0};
    xfile_basename(really_filename, filename);
    return !strcmp(filename, really_filename);
}

bool xfile_exists(char const *filepath) { return access(filepath, F_OK) == 0; }

enum sched_rc xfile_touch(char const *filepath)
{
    if (xfile_exists(filepath)) return SCHED_OK;
    int fd = open(filepath, O_RDWR | O_CREAT,
                  S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP);
    if (fd == -1) return error(SCHED_FAIL_OPEN_FILE);
    if (close(fd) == -1) return error(SCHED_FAIL_CLOSE_FILE);
    return SCHED_OK;
}
