/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include "clip.h"
#include <stdio.h>
#include <stdlib.h>
#include <Carbon/Carbon.h>
#include "platform/clipboard.h"
#include <r9k/string.h>

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

void clip_watch(PFN_clip_watch watch)
{
        char *prev = NULL;

        if (!watch)
                return;

        while (1) {
                char *cur = clip_read();

                if (cur && !strblank(cur) && (!prev || strcmp(cur, prev) != 0)) {
                        watch(cur);
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