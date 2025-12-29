/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 varketh
 */
#ifndef PANIC_H_
#define PANIC_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <r9k/compiler_attrs.h>

__attr_noreturn
__attr_printf(1, 2)
static void _panic(const char *fmt, ...)
{
        va_list va;
        va_start(va, fmt);
        vfprintf(stderr, fmt, va);
        va_end(va);
        exit(EXIT_FAILURE);
}

#define PANIC(fmt, ...) _panic(fmt, ##__VA_ARGS__)

#define PANIC_IF(cond, fmt, ...)                        \
        do {                                            \
                if (cond)                               \
                        _panic(fmt, ##__VA_ARGS__);     \
        } while(0)

#endif /* PANIC_H_ */