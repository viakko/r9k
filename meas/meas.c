/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <argparser.h>
#include <ctype.h>
#include <string.h>
#include <fx/typedefs.h>

#define MEAS_VERSION "1.0.0"

static size_t __strlen_utf8(const char *str) // NOLINT(*-reserved-identifier)
{
        size_t len = 0;

        while (*str) {
                unsigned char c = (unsigned char) *str;
                str++;

                if (isspace(c))
                        continue;

                if ((c & 0xC0) != 0x80)
                        len++;
        }

        return len;
}

static size_t length(const char *str, bool is_unicode)
{
        return is_unicode ? __strlen_utf8(str) : strlen(str);
}

static int on_help(struct argparser *ap, struct option *opt)
{
        printf("%s\n", argparser_help(ap));
        exit(0);
}

static int on_version(struct argparser *ap, struct option *opt)
{
        printf("meas %s\n", MEAS_VERSION);
        exit(0);
}

static int on_str(struct argparser *ap, struct option *opt)
{
        printf("%zu\n", length(opt->sval, argparser_find(ap, "unicode")));
        exit(0);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help;
        struct option *version;
        struct option *str;
        struct option *unicode;

        ap = argparser_create("meas");
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", NULL, opt_none);
        argparser_add0(ap, &version, "version", NULL, "show current version", NULL, opt_none);
        argparser_add1(ap, &str, "s", "str", "as string type", on_str, opt_reqval);
        argparser_add0(ap, &unicode, "u", "unicode", "use unicode parse string length", NULL, opt_none);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        /* default */
        if (argparser_count(ap) > 0)
                printf("%zu\n", length(argparser_val(ap, 0), !IS_NULL(unicode)));

        argparser_free(ap);

        return 0;
}
