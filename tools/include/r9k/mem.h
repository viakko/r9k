/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 varketh
 */
#ifndef MEM_H_
#define MEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <r9k/compiler_attrs.h>

__attr_unused
static void *_malloc_trace(size_t size, const char *filename, int line)
{
        void *ptr = malloc(size);
        printf("[mem] %s:%d _malloc_trace %p %zu\n", filename, line, ptr, size);
        return ptr;
}

static void *_calloc_trace(size_t count, size_t size, const char *filename, int line)
{
        void *ptr = calloc(count, size);
        printf("[mem] %s:%d _calloc_trace %p %zu\n", filename, line, ptr, size);
        return ptr;
}

static void *_realloc_trace(void *ptr, size_t size, const char *filename, int line)
{
        void *reptr = realloc(ptr, size);
        printf("[mem] %s:%d _realloc_trace %p -> %p %zu\n", filename, line, ptr, reptr, size);
        return reptr;
}

static void _free_trace(void *ptr, const char *filename, int line)
{
        if (ptr == NULL)
                return;
        printf("[mem] %s:%d _free_trace %p\n", filename, line, ptr);
        free(ptr);
}

#define malloc(size) _malloc_trace(size, __FILE__, __LINE__)
#define calloc(count, size) _calloc_trace(count, size, __FILE__, __LINE__)
#define realloc(ptr, size) _realloc_trace(ptr, size, __FILE__, __LINE__)
#define free(ptr) _free_trace(ptr, __FILE__, __LINE__)

#endif /* MEM_H_ */