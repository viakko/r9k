/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 varketh
 */
#include <r9k/string.h>
#include <ctype.h>

int is_str_blank(const char *str)
{
        if (!str)
                return 1;

        while (*str) {
                if (!isspace(*str))
                        return 0;
                str++;
        }

        return 0;
}