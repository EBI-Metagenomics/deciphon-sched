// Amalgamation of the following files:
//    zc_byteswap.h
//    zc_endian.h
//    zc_limits.h
//    zc_memory.h
//    zc_mempool.h
//    zc_os.h
//    zc_path.h
//    zc_pp.h
//    zc_string.h
/* --- zc_byteswap section -------------------------------- */

#ifndef ZC_BYTESWAP_H
#define ZC_BYTESWAP_H

// Acknowledgment: musl

#include <stdint.h>

uint16_t zc_byteswap16(uint16_t x);
uint32_t zc_byteswap32(uint32_t x);
uint64_t zc_byteswap64(uint64_t x);

#endif

/* --- zc_endian section ---------------------------------- */

#ifndef ZC_ENDIAN_H
#define ZC_ENDIAN_H

#include <stdint.h>

uint16_t zc_htons(uint16_t);
uint32_t zc_htonl(uint32_t);
uint64_t zc_htonll(uint64_t);

uint16_t zc_ntohs(uint16_t);
uint32_t zc_ntohl(uint32_t);
uint64_t zc_ntohll(uint64_t);

#endif

/* --- zc_limits section ---------------------------------- */

#ifndef ZC_LIMITS_H
#define ZC_LIMITS_H

#include <inttypes.h>
#include <limits.h>

#if SHRT_MAX == INT8_MAX
#define ZC_BYTES_PER_SHORT 1
#define ZC_SHORT_WIDTH 8
#elif SHRT_MAX == INT16_MAX
#define ZC_BYTES_PER_SHORT 2
#define ZC_SHORT_WIDTH 16
#elif SHRT_MAX == INT32_MAX
#define ZC_BYTES_PER_SHORT 4
#define ZC_SHORT_WIDTH 32
#elif SHRT_MAX == INT64_MAX
#define ZC_BYTES_PER_SHORT 8
#define ZC_SHORT_WIDTH 64
#else
#error "Cannot determine size of short"
#endif

#if INT_MAX == INT8_MAX
#define ZC_BYTES_PER_INT 1
#define ZC_INT_WIDTH 8
#elif INT_MAX == INT16_MAX
#define ZC_BYTES_PER_INT 2
#define ZC_INT_WIDTH 16
#elif INT_MAX == INT32_MAX
#define ZC_BYTES_PER_INT 4
#define ZC_INT_WIDTH 32
#elif INT_MAX == INT64_MAX
#define ZC_BYTES_PER_INT 8
#define ZC_INT_WIDTH 64
#else
#error "Cannot determine size of int"
#endif

#if LONG_MAX == INT8_MAX
#define ZC_BYTES_PER_LONG 1
#define ZC_LONG_WIDTH 8
#elif LONG_MAX == INT16_MAX
#define ZC_BYTES_PER_LONG 2
#define ZC_LONG_WIDTH 16
#elif LONG_MAX == INT32_MAX
#define ZC_BYTES_PER_LONG 4
#define ZC_LONG_WIDTH 32
#elif LONG_MAX == INT64_MAX
#define ZC_BYTES_PER_LONG 8
#define ZC_LONG_WIDTH 64
#else
#error "Cannot determine size of long"
#endif

#endif

/* --- zc_memory section ---------------------------------- */

#ifndef ZC_MEMORY_H
#define ZC_MEMORY_H

#include <stddef.h>

void *zc_reallocf(void *ptr, size_t size);
void zc_bzero(void *dst, size_t dsize);

#endif

/* --- zc_mempool section --------------------------------- */

#ifndef ZC_MEMPOOL_H
#define ZC_MEMPOOL_H

#include <stddef.h>

struct mempool;

struct mempool *zc_mempool_new(unsigned bits, size_t object_size);
void zc_mempool_del(struct mempool *);

void *zc_mempool_new_object(struct mempool *);
void zc_mempool_del_object(struct mempool *, void *object);

#endif

/* --- zc_os section -------------------------------------- */

#ifndef ZC_OS_H
#define ZC_OS_H

enum
{
    ZC_WINDOWS,
    ZC_UNIX,
    ZC_MACOS,
};

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
#define ZC_OS ZS_WINDOWS
#elif defined(__unix__) || defined(__unix)
#define ZC_OS ZS_UNIX
#elif defined(__APPLE__)
#define ZC_OS ZC_MACOS
#endif

#ifdef MS_WINDOWS
#define ZC_PATH_SEP '\\'
#endif

#ifndef ZC_PATH_SEP
#define ZC_PATH_SEP '/'
#endif

#endif

/* --- zc_path section ------------------------------------ */

#ifndef ZC_PATH_H
#define ZC_PATH_H

char *zc_basename(char *path);
char *zc_dirname(char *path);

#endif

/* --- zc_pp section -------------------------------------- */

#ifndef ZZ_PP_H
#define ZZ_PP_H

#define zc_array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * zc_container_of - cast a member of a structure out to the containing
 * structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define zc_container_of(ptr, type, member)                                     \
    ({                                                                         \
        char *__mptr = (char *)(ptr);                                          \
        ((type *)(__mptr - offsetof(type, member)));                           \
    })

/**
 * zc_container_of_safe - cast a member of a structure out to the containing
 * structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 * Return NULL if ptr is NULL.
 */
#define zc_container_of_safe(ptr, type, member)                                \
    ({                                                                         \
        char *__mptr = (char *)(ptr);                                          \
        __mptr == NULL ? (type *)__mptr                                        \
                       : ((type *)(__mptr - offsetof(type, member)));          \
    })

#endif

/* --- zc_string section ---------------------------------- */

#ifndef ZC_STRING_H
#define ZC_STRING_H

#include <stddef.h>

char *zc_strdup(const char *s);
size_t zc_strlcat(char *dst, char const *src, size_t dsize);
size_t zc_strlcpy(char *dst, char const *src, size_t dsize);

#endif
