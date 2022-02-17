#include "xfile.h"
#include "bug.h"
#include "compiler.h"
#include "limits.h"
#include "logger.h"
#include "sched/limits.h"
#include "strlcpy.h"
#include "xxhash/xxhash.h"
#include <assert.h>
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

enum sched_rc xfile_size(char const *filepath, int64_t *size)
{
    struct stat st = {0};
    if (stat(filepath, &st) == 1) return eio("stat");
    assert(sizeof(st.st_size) == 8);
    off_t sz = st.st_size;
    *size = (int64_t)sz;
    return SCHED_OK;
}

enum sched_rc xfile_psize(FILE *fp, int64_t *size)
{
    off_t old = ftello(fp);
    if (old == -1) return eio("ftello");
    if (fseeko(fp, 0, SEEK_END) == -1) return eio("fseeko");
    off_t sz = ftello(fp);
    if (sz == -1) return eio("fseeko");
    if (fseeko(fp, old, SEEK_SET) == -1) return eio("fseeko");
    *size = (int64_t)sz;
    return SCHED_OK;
}

enum sched_rc xfile_dsize(int fd, int64_t *size)
{
    struct stat st = {0};
    if (fsync(fd) == -1) return eio("fsync");
    if (fstat(fd, &st) == 1) return eio("fstat");
    assert(sizeof(st.st_size) == 8);
    off_t sz = st.st_size;
    *size = (int64_t)sz;
    return SCHED_OK;
}

enum sched_rc xfile_hash(FILE *restrict fp, uint64_t *hash)
{
    int rc = SCHED_EFAIL;
    XXH64_state_t *const state = XXH64_createState();
    if (!state)
    {
        rc = error(SCHED_EFAIL, "failed to create state");
        goto cleanup;
    }
    XXH64_reset(state, 0);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = eio("fread");
            goto cleanup;
        }

        XXH64_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = eio("fread");
        goto cleanup;
    }

    rc = SCHED_OK;
    *hash = XXH64_digest(state);

cleanup:
    XXH64_freeState(state);
    return rc;
}

enum sched_rc xfile_tmp_open(struct xfile_tmp *file)
{
    strlcpy(file->path, XFILE_PATH_TEMP_TEMPLATE, ARRAY_SIZE_OF(*file, path));
    file->fp = 0;

    enum sched_rc rc = xfile_mktemp(file->path);
    if (rc) return rc;

    if (!(file->fp = fopen(file->path, "wb+"))) return eio("fopen");

    return SCHED_OK;
}

void xfile_tmp_del(struct xfile_tmp const *file)
{
    fclose(file->fp);
    remove(file->path);
}

enum sched_rc xfile_copy(FILE *restrict dst, FILE *restrict src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src)) return eio("fread");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return eio("fwrite");
    }
    if (ferror(src)) return eio("fread");

    return SCHED_OK;
}

bool xfile_is_readable(char const *filepath)
{
    FILE *file = NULL;
    if ((file = fopen(filepath, "r")))
    {
        fclose(file);
        return true;
    }
    return false;
}

enum sched_rc xfile_mktemp(char *filepath)
{
    if (mkstemp(filepath) == -1) return error(SCHED_EIO, "mkstemp failed");
    return SCHED_OK;
}

static char *glibc_basename(const char *filename)
{
    char *p = strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
}

static enum sched_rc append_ext(char *str, size_t len, size_t max_size,
                                char const *ext)
{
    char *j = &str[len];
    size_t n = strlen(ext);
    if (n + 1 + (size_t)(j - str) > max_size)
        return error(SCHED_ENOMEM, "not enough memory");
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *j = *ext;
    return SCHED_OK;
}

static enum sched_rc change_ext(char *str, size_t pos, size_t max_size,
                                char const *ext)
{
    char *j = &str[pos];
    while (j > str && *j != '.')
        --j;
    if (j == str) return SCHED_EFAIL;
    return append_ext(str, (size_t)(j - str), max_size, ext);
}

enum sched_rc xfile_set_ext(size_t max_size, char *str, char const *ext)
{
    size_t len = strlen(str);
    if (change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return SCHED_OK;
}

void xfile_basename(char *filename, char const *path)
{
    char *p = glibc_basename(path);
    strlcpy(filename, p, FILENAME_SIZE);
}

void xfile_strip_ext(char *str)
{
    char *ret = strrchr(str, '.');
    if (ret) *ret = 0;
}

enum sched_rc xfile_filepath_from_fptr(FILE *fp, char *filepath)
{
    int fd = fileno(fp);
#ifdef __APPLE__
    if (fcntl(fd, F_GETPATH, filepath) == -1) return eio("F_GET_PATH");
#else
    sprintf(filepath, "/proc/self/fd/%d", fd);
#endif
    return SCHED_OK;
}

FILE *xfile_open_from_fptr(FILE *fp, char const *mode)
{
    char filepath[PATH_MAX] = {0};
    enum sched_rc rc = xfile_filepath_from_fptr(fp, filepath);
    if (rc) return NULL;
    return fopen(filepath, mode);
}

bool xfile_exists(char const *filepath) { return access(filepath, F_OK) == 0; }

enum sched_rc xfile_touch(char const *filepath)
{
    if (xfile_exists(filepath)) return SCHED_OK;
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return eio("fopen");
    if (fclose(fp)) return eio("fclose");
    return SCHED_OK;
}
