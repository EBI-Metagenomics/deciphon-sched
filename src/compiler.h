#ifndef COMPILER_H
#define COMPILER_H

#define STRINGIFY(s) __STRINGIFY(s)
#define __STRINGIFY(s) #s

#ifdef __FILE_NAME__
#define LOCAL __FILE_NAME__ ":" STRINGIFY(__LINE__)
#else
#define LOCAL __FILE__ ":" STRINGIFY(__LINE__)
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MEMBER_REF(var, member) ((__typeof__(var) *)0)->member
#define MEMBER_SIZE(var, member) sizeof(MEMBER_REF((var), member))
#define ARRAY_SIZE_OF(var, member) ARRAY_SIZE(MEMBER_REF((var), member))

/*
 * __has_builtin is supported on gcc >= 10, clang >= 3 and icc >= 21.
 * In the meantime, to support gcc < 10, we implement __has_builtin
 * by hand.
 */
#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

#if !__has_builtin(__builtin_unreachable)
#define __builtin_unreachable() (void)(0)
#endif

/* Are two types/vars the same type (ignoring qualifiers)? */
#define same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define unused(arg) (void)arg;

#endif
