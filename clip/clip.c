/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include "clip.h"
//std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//r9k
#include <r9k/typedefs.h>

int clip_write(const char *text)
{
        FILE* pipe;

        if (!text || strlen(text) == 0)
                return 0;

        pipe = popen("pbcopy", "w");
        if (!pipe)
                return -1;

        size_t len = strlen(text);
        size_t written = fwrite(text, 1, len, pipe);

        if (written != len) {
                perror("fwrite failed");
                pclose(pipe);
                return -1;
        }

        pclose(pipe);
        return 0;
}

char *clip_read()
{
        FILE *pipe = popen("pbpaste", "r");
        if (!pipe)
                return NULL;

        char *buf = NULL;
        size_t cap = 256;
        size_t pos = 0;
        size_t n;

        buf = malloc(cap);
        if (!buf) {
                pclose(pipe);
                return NULL;
        }

        while (1) {
                if (pos >= cap) {
                        cap = cap + (cap >> 1);
                        char *tmp = realloc(buf, cap + 1);
                        if (!tmp)
                                goto err;

                        buf = tmp;
                }

                n = fread(buf + pos, 1, cap - pos, pipe);
                if (n == 0) {
                        if (ferror(pipe))
                                goto err;
                        break;
                }

                pos += n;
        }

        buf[pos] = '\0';

        pclose(pipe);
        return buf;

err:
        pclose(pipe);
        free(buf);
        return NULL;
}