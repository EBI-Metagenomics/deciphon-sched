#include "array.h"
#include "safe.h"
#include <stdlib.h>
#include <string.h>

struct array
{
    size_t size;
    size_t capacity;
    char data[];
};

size_t array_size(struct array const *arr) { return arr->size; }

char *array_data(struct array *arr) { return arr->data; }

struct array *array_new(size_t size)
{
    struct array *arr = malloc(sizeof(*arr) + sizeof(char) * size);
    if (arr)
    {
        arr->size = size;
        arr->capacity = size;
    }
    return arr;
}

void array_del(struct array const *arr) { free((void *)arr); }

struct array *array_prealloc(struct array *arr, size_t size)
{
    if (arr->capacity >= size) return arr;

    struct array *dst = safe_realloc(arr, sizeof(*dst) + sizeof(char) * size);
    if (dst)
    {
        dst->size = size;
        dst->capacity = size;
    }
    return dst;
}

struct array *array_put(struct array *arr, char const *data, size_t size)
{
    struct array *dst = array_prealloc(arr, size);
    if (dst)
    {
        dst->size = size;
        memcpy(dst->data, data, size);
    }
    return dst;
}

struct array *array_shrink(struct array *arr)
{
    if (arr->capacity == arr->size) return arr;

    struct array *dst =
        safe_realloc(arr, sizeof(*dst) + sizeof(char) * arr->size);
    if (dst)
    {
        dst->size = arr->size;
        dst->capacity = arr->size;
    }
    return dst;
}
