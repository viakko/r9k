/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include <r9k/panic.h>
#include <stdio.h>
#include <stdlib.h>

void vpanic(const char *fmt, va_list va)
{
        vfprintf(stderr, fmt, va);
        exit(EXIT_FAILURE);
}

void panic(const char *fmt, ...)
{
        va_list va;
        va_start(va, fmt);
        vpanic(fmt, va);
        va_end(va);
}