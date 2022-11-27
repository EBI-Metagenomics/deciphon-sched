
#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int fs_readall(char const *filepath, long *size, unsigned char **data);
bool fs_exists(char const *filepath);

#endif
