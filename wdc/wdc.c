/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <argparser.h>
#include <string.h>
#include <r9k/typedefs.h>

#include "r9k/ioutils.h"

#define MEAS_VERSION "1.0"

static size_t __strlen_utf8(const char *str) // NOLINT(*-reserved-identifier)
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

static size_t length(const char *str, bool character)
{
        return character ? __strlen_utf8(str) : strlen(str);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help;
        struct option *version;
        struct option *character;

        ap = argparser_create("meas", MEAS_VERSION);
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &version, "version", NULL, "show current version", __acb_version, opt_none);
        argparser_add0(ap, &character, "c", NULL, "as character count", NULL, opt_none);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        /* default */
        if (argparser_count(ap) > 0) {
                printf("%zu\n", length(argparser_val(ap, 0), !IS_NULL(character)));
                goto end;
        }

        char *buf = readin();
        if (buf) {
                printf("%zu\n", length(buf, !IS_NULL(character)));
                free(buf);
        }

end:
        argparser_free(ap);

        return 0;
}
