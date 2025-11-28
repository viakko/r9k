/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

static char *readfile(const char *path)
{
        FILE *fp;
        size_t size;
        char *buf;

        fp = fopen(path, "rb");
        if (!fp)
                return NULL;

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

        fclose(fp);
        return buf;
}

#endif /* IO_UTILS_H_ */