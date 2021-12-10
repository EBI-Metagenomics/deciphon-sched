#ifndef SAFE_H
#define SAFE_H

#include <stddef.h>

void *safe_realloc(void *ptr, size_t size);
size_t safe_strcpy(char *restrict dst, char const *restrict src,
                   size_t dst_size);

#endif
