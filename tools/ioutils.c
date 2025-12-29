/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 varketh
 */
#include <r9k/ioutils.h>
//std
#include <stdlib.h>
#include <string.h>

char *readall(FILE *fp)
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

        if (!buf)
                return NULL;

        buf[len] = '\0';

        return buf;
}

char *readfile(const char *path)
{
        char *buf;
        FILE *fp;

        fp = fopen(path, "r");
        if (!fp)
                return NULL;

        buf = readall(fp);
        fclose(fp);

        return buf;
}

char *readin()
{
        return readall(stdin);
}
