/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <r9k/argparser.h>
#include <r9k/typedefs.h>
#include <r9k/string.h>

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

static int cb_count_chracters(struct argparser *ap, struct option *countchr)
{
        (void) countchr;
        if (argparser_count(ap) > 0)
                printf("%zu\n", utf8len(argparser_val(ap, 0)));
        exit(0);
}

int main(int argc, char* argv[])
{
        struct argparser *ap;
        struct option *help;
        struct option *version;
        struct option *countchr;

        ap = argparser_create("strl", "1.0.0");
        if (!ap)
                die("argparser initialize failed");

        argparser_add0(ap, &help, "h", "help", "show this help message.", ACB_HELP, 0);
        argparser_add0(ap, &version, "version", NULL, "show current version.", ACB_VERSION, 0);
        argparser_add0(ap, &countchr, "c", NULL, "count characters by unicode.", cb_count_chracters, 0);

        if (argparser_run(ap, argc, argv) != 0)
                die("%s", argparser_error(ap));

        if (argparser_count(ap) > 0)
                printf("%zu\n", strlen(argparser_val(ap, 0)));

        argparser_free(ap);

        return 0;
}