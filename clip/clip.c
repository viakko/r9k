/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include "clip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Carbon/Carbon.h>
#include "platform/clipboard.h"

static int strblank(const char *str)
{
        return !str || !*str || strspn(str, "\t\r\n") == strlen(str);
}

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

char *clip_read(void)
{
        return __clip_read();
}

void clip_watch(void)
{
        char *prev = NULL;

        while (1) {
                char *cur = clip_read();

                if (cur && !strblank(cur) && (!prev || strcmp(cur, prev) != 0)) {
                        printf("%s\n", cur);
                        free(prev);
                        prev = cur;
                        goto sleep;
                }

                free(cur);
sleep:
                sleep(1);
        }

        free(prev);
}