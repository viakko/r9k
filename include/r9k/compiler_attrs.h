/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 varketh
 */
#ifndef COMPILER_ATTRUBUTES_H_
#define COMPILER_ATTRUBUTES_H_

#define __attr_ignore(x) ((void)(x))
#define __attr_ignore2(a, b) __attr_ignore(a), __attr_ignore(b)

#if defined(__GNUC__) || defined(__clang__)
#  define __attr_nonnull(...)  __attribute__((nonnull(__VA_ARGS__)))
#  define __attr_unused        __attribute__((unused))
#  define __attr_noreturn      __attribute__((noreturn))
#  define __attr_printf(a, b)  __attribute__((format(printf, a, b)))
#  define __attr_always_inline __attribute__((always_inline))
#else
#  define __attr_nonnull(...)
#  define __attr_unused
#  define __attr_noreturn
#  define __attr_printf(a, b)
#  define __attr_always_inline
#endif

#endif /* COMPILER_ATTRUBUTES_H_ */