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

enum sched_rc xfile_hash(FILE *restrict fp, uint64_t *hash);
bool xfile_is_name(char const *filename);

bool xfile_exists(char const *filepath);
enum sched_rc xfile_touch(char const *filepath);

#endif
