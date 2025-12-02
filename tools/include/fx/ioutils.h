/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

static char *__fp_readall(FILE *fp)
{
        size_t size;
        char *buf;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);

        buf = malloc(size + 1);
        if (!buf) {
                fclose(fp);
                return NULL;
        }

        fread(buf, 1, size, fp);
        buf[size] = '\0';

        return buf;
}

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

static char *readin()
{
        static char *stdin_buf = NULL;

        if (stdin_buf)
                return stdin_buf;

        stdin_buf = __fp_readall(stdin);

        return stdin_buf;
}

#endif /* IO_UTILS_H_ */