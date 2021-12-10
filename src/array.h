#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

struct array;

size_t array_size(struct array const *arr);

char *array_data(struct array *arr);

struct array *array_new(size_t size);

void array_del(struct array const *arr);

struct array *array_prealloc(struct array *arr, size_t size);

struct array *array_put(struct array *arr, char const *data, size_t size);

struct array *array_shrink(struct array *arr);

#endif
