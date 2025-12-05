/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include "charc.h"
//std
#include <string.h>

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str) {
                unsigned char c = (unsigned char) *str;
                str++;
                if ((c & 0xC0) != 0x80)
                        len++;
        }

        return len;
}

static size_t length(const char *str, bool _char)
{
        return _char ? utf8len(str) : strlen(str);
}

size_t linec(const char *str)
{
        size_t count = 0;

        while (*str) {
                if (*str == '\n')
                        count++;
                str++;
        }

        return count;
}

size_t charc(const char *str, bool is_utf8)
{
        return length(str, is_utf8);
}