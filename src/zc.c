// Amalgamation of the following files:
//    zc_byteswap.c
//    zc_endian.c
//    zc_memory.c
//    zc_mempool.c
//    zc_path.c
//    zc_string.c

#include "zc.h"

/* --- zc_byteswap section -------------------------------- */

uint16_t zc_byteswap16(uint16_t x) { return (uint16_t)(x << 8 | x >> 8); }

uint32_t zc_byteswap32(uint32_t x)
{
    return (uint32_t)(x >> 24 | (x >> 8 & 0xff00) | (x << 8 & 0xff0000) |
                      x << 24);
}

uint64_t zc_byteswap64(uint64_t x)
{
    return (uint64_t)((zc_byteswap32(x) + 0ULL) << 32 | zc_byteswap32(x >> 32));
}

/* --- zc_endian section ---------------------------------- */

#include <stdlib.h>

enum
{
    ZC_ENDIAN_LITTLE = 1234,
    ZC_ENDIAN_BIG = 4321,
};

#ifdef __STDC_ENDIAN_NATIVE__
#if __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__
#define ZC_ENDIAN_NATIVE ZC_ENDIAN_LITTLE
#elif __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__
#define ZC_ENDIAN_NATIVE ZC_ENDIAN_BIG
#else
#error "Unknown native byte order"
#endif
#endif

#ifdef ZC_ENDIAN_NATIVE

#if ZC_ENDIAN_NATIVE == ZC_ENDIAN_LITTLE

uint16_t zc_htons(uint16_t x) { return zc_byteswap16(x); }
uint32_t zc_htonl(uint32_t x) { return zc_byteswap32(x); }
uint64_t zc_htonll(uint64_t x) { return zc_byteswap64(x); }

uint16_t zc_ntohs(uint16_t x) { return zc_htons(x); }
uint32_t zc_ntohl(uint32_t x) { return zc_htonl(x); }
uint64_t zc_ntohll(uint64_t x) { return zc_htonll(x); }

#else
uint16_t zc_htons(uint16_t x) { return x; }
uint32_t zc_htonl(uint32_t x) { return x; }
uint64_t zc_htonll(uint64_t x) { return x; }

uint16_t zc_ntohs(uint16_t x) { return x; }
uint32_t zc_ntohl(uint32_t x) { return x; }
uint64_t zc_ntohll(uint64_t x) { return x; }
#endif

#else

static int zc_endianness(void)
{
    union
    {
        uint32_t value;
        uint8_t data[sizeof(uint32_t)];
    } number;

    number.data[0] = 0x00;
    number.data[1] = 0x01;
    number.data[2] = 0x02;
    number.data[3] = 0x03;

    switch (number.value)
    {
    case (uint32_t)(0x00010203):
        return ZC_ENDIAN_BIG;
    case (uint32_t)(0x03020100):
        return ZC_ENDIAN_LITTLE;
    default:
        exit(1);
        return -1;
    }
}

uint16_t zc_htons(uint16_t x)
{
    if (zc_endianness() == ZC_ENDIAN_BIG) return x;
    return zc_byteswap16(x);
}

uint32_t zc_htonl(uint32_t x)
{
    if (zc_endianness() == ZC_ENDIAN_BIG) return x;
    return zc_byteswap32(x);
}

uint64_t zc_htonll(uint64_t x)
{
    if (zc_endianness() == ZC_ENDIAN_BIG) return x;
    return zc_byteswap64(x);
}

uint16_t zc_ntohs(uint16_t x) { return zc_htons(x); }
uint32_t zc_ntohl(uint32_t x) { return zc_htonl(x); }
uint64_t zc_ntohll(uint64_t x) { return zc_htonll(x); }

#endif

/* --- zc_memory section ---------------------------------- */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void *zc_reallocf(void *ptr, size_t size)
{
    assert(size > 0);
    void *new_ptr = realloc(ptr, size);

    if (!new_ptr) free(ptr);
    return new_ptr;
}

void zc_bzero(void *dst, size_t dsize) { memset(dst, 0, dsize); }

/* --- zc_mempool section --------------------------------- */

/* Acknowledgment: https://github.com/dcreager/libcork */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

struct proxy
{
    struct proxy *next_free;
};

struct block
{
    struct block *next_block;
};

struct mempool
{
    size_t object_size;
    size_t block_size;
    struct proxy *free_list;
    size_t allocated_count;
    struct block *blocks;
};

/* chunk is the memory allocated to fit both proxy and its object */
static inline size_t object_chunk_size(struct mempool const *mp)
{
    return (sizeof(struct proxy) + (mp)->object_size);
}

static inline struct proxy *get_proxy_from_object(void *object)
{
    return (struct proxy *)(object)-1;
}

static inline void *get_object_from_proxy(struct proxy *proxy)
{
    return ((struct proxy *)(proxy)) + 1;
}

struct mempool *zc_mempool_new(unsigned bits, size_t object_size)
{
    struct mempool *mp = malloc(sizeof(struct mempool));
    if (!mp) return 0;
    mp->object_size = object_size;
    mp->block_size = 1 << (bits);
    mp->free_list = 0;
    mp->allocated_count = 0;
    mp->blocks = 0;
    assert(object_chunk_size(mp) <= mp->block_size);
    return mp;
}

void zc_mempool_del(struct mempool *mp)
{
    assert(mp->allocated_count == 0);

    struct block *curr = mp->blocks;
    while (curr)
    {
        struct block *next = curr->next_block;
        free(curr);
        curr = next;
    }

    free(mp);
}

static void divide_block_into_chunks(struct mempool *mp, struct block *block)
{
    void *offset = block;
    size_t pos = sizeof(struct block);
    while (pos + object_chunk_size(mp) <= mp->block_size)
    {
        struct proxy *proxy = offset + pos;
        proxy->next_free = mp->free_list;
        mp->free_list = proxy;
        pos += object_chunk_size(mp);
    }
}

static bool new_block(struct mempool *mp)
{
    struct block *block = malloc(mp->block_size);
    if (!block) return false;
    block->next_block = mp->blocks;
    mp->blocks = block;

    divide_block_into_chunks(mp, block);
    return true;
}

void *zc_mempool_new_object(struct mempool *mp)
{
    if (!mp->free_list && !new_block(mp)) return 0;

    struct proxy *obj = mp->free_list;
    mp->free_list = obj ? obj->next_free : 0;
    mp->allocated_count++;
    return get_object_from_proxy(obj);
}

void zc_mempool_del_object(struct mempool *mp, void *object)
{
    struct proxy *proxy = get_proxy_from_object(object);
    proxy->next_free = mp->free_list;
    mp->free_list = proxy;
    mp->allocated_count--;
}

/* --- zc_path section ------------------------------------ */

#include <stddef.h>
#include <string.h>

// Acknowledgment: gblic
char *zc_basename(char *path)
{
    char *p = strrchr(path, ZC_PATH_SEP);
    return p ? p + 1 : path;
}

char *zc_dirname(char *path)
{
    char *p = strrchr(path, ZC_PATH_SEP);
    while (p > path)
    {
        *p-- = 0;
    }
    return path;
}

/* --- zc_string section ---------------------------------- */

#include <stdlib.h>
#include <string.h>

char *zc_strdup(char const *str)
{
    size_t len = strlen(str) + 1;
    void *new = malloc(len);

    if (new == NULL) return NULL;

    return (char *)memcpy(new, str, len);
}

/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 *
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 */
size_t zc_strlcat(char *dst, char const *src, size_t dsize)
{
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dsize;
    size_t dlen;

    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odst;
    n = dsize - dlen;

    if (n-- == 0) return (dlen + strlen(src));
    while (*src != '\0')
    {
        if (n != 0)
        {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return (dlen + (src - osrc));
}

/*
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
size_t zc_strlcpy(char *dst, char const *src, size_t dsize)
{
    char const *osrc = src;
    size_t nleft = dsize;

    if (nleft != 0)
    {
        while (--nleft != 0)
        {
            if ((*dst++ = *src++) == '\0') break;
        }
    }

    if (nleft == 0)
    {
        if (dsize != 0) *dst = '\0';
        while (*src++)
            ;
    }

    return (size_t)(src - osrc - 1);
}
