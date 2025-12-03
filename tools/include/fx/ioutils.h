/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#define readin() __fp_readall(stdin)

static char *__fp_readall(FILE *fp)
{
        char *buf = NULL;
        char *tmp;
        size_t len = 0;
        size_t cap = 0;
        char chunk[4096];
        size_t n;

        while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
                if (len + n + 1 > cap) {
                        cap = cap ? cap + (cap >> 1) : sizeof(chunk) * 2;
                        if (cap < len + n + 1)
                                cap = len + n + 1;

                        tmp = realloc(buf, cap);
                        if (!tmp) {
                                if (buf)
                                        free(buf);
                                return NULL;
                        }
                        buf = tmp;
                }
                memcpy(buf + len, chunk, n);
                len += n;
        }

        if (!buf) {
                buf = malloc(1);
                if (!buf)
                        return NULL;
        }

        buf[len] = '\0';

        return buf;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static char *readfile(const char *path)
{
        char *buf;
        FILE *fp;

        fp = fopen(path, "r");
        if (!fp)
                return NULL;

        buf = __fp_readall(fp);
        fclose(fp);

        return buf;
}
#pragma GCC diagnostic pop

#endif /* IO_UTILS_H_ */