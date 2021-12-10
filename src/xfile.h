#ifndef XFILE_H
#define XFILE_H

#include <stdint.h>
#include <stdio.h>

#define XFILE_PATH_TEMP_TEMPLATE "/tmp/dcpXXXXXX"

struct xfile_tmp
{
    char path[sizeof XFILE_PATH_TEMP_TEMPLATE];
    FILE *fp;
};

int xfile_mktemp(char *filepath);
int xfile_hash(FILE *restrict fp, uint64_t *hash);

int xfile_tmp_open(struct xfile_tmp *file);
int xfile_tmp_rewind(struct xfile_tmp *file);
void xfile_tmp_del(struct xfile_tmp const *file);

#endif
