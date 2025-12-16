/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef PANIC_H_
#define PANIC_H_

#include <stdarg.h>

void vpanic(const char *fmt, va_list va);
void panic(const char *fmt, ...);
void panic_if(int cond, const char *fmt, ...);

#endif /* PANIC_H_ */