/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef _PANIC_H_
#define _PANIC_H_

#include <stdarg.h>

void vpanic(const char *fmt, va_list va);
void panic(const char *fmt, ...);

#define PANIC_IF(cond, fmt, ...)        \
        do {                            \
                if (cond)               \
                        panic(fmt, ##__VA_ARGS__); \
        } while(0)

#endif /* _PANIC_H_ */