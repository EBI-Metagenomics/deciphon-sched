#ifndef XFILE_H
#define XFILE_H

#include "limits.h"
#include "sched/rc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define XFILE_PATH_TEMP_TEMPLATE "/tmp/dcpXXXXXX"

struct xfile_tmp
{
    char path[sizeof XFILE_PATH_TEMP_TEMPLATE];
    FILE *fp;
};

enum sched_rc xfile_size(char const *filepath, int64_t *size);
enum sched_rc xfile_psize(FILE *fp, int64_t *size);
enum sched_rc xfile_dsize(int fd, int64_t *size);
enum sched_rc xfile_hash(FILE *restrict fp, uint64_t *hash);

enum sched_rc xfile_tmp_open(struct xfile_tmp *file);
void xfile_tmp_del(struct xfile_tmp const *file);

enum sched_rc xfile_copy(FILE *restrict dst, FILE *restrict src);
bool xfile_is_readable(char const *filepath);
enum sched_rc xfile_mktemp(char *filepath);

enum sched_rc xfile_set_ext(size_t max_size, char *str, char const *ext);
void xfile_basename(char *filename, char const *path);
void xfile_strip_ext(char *str);

enum sched_rc xfile_filepath_from_fptr(FILE *fp, char *filepath);
FILE *xfile_open_from_fptr(FILE *fp, char const *mode);

bool xfile_exists(char const *filepath);
enum sched_rc xfile_touch(char const *filepath);

#endif
