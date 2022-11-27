#include "fs.h"
#include "sched/rc.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fs_size(char const *filepath, long *size)
{
    struct stat st = {0};
    if (stat(filepath, &st) == -1) return SCHED_FAIL_STAT_FILE;
    *size = (long)st.st_size;
    return 0;
}

int fs_readall(char const *filepath, long *size, unsigned char **data)
{
    *size = 0;
    *data = NULL;
    int rc = fs_size(filepath, size);
    if (rc) return rc;

    if (*size == 0) return 0;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return SCHED_FAIL_OPEN_FILE;

    if (!(*data = malloc(*size)))
    {
        fclose(fp);
        return SCHED_NOT_ENOUGH_MEMORY;
    }

    if (fread(*data, *size, 1, fp) < 1)
    {
        fclose(fp);
        free(*data);
        *data = NULL;
        return SCHED_FAIL_READ_FILE;
    }

    return fclose(fp) ? SCHED_FAIL_CLOSE_FILE : 0;
}

bool fs_exists(char const *filepath) { return access(filepath, F_OK) == 0; }
