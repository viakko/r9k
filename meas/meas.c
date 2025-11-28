/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <argparser.h>
#include <string.h>

#define MEAS_VERSION "1.0"

static size_t strlen_utf8(const char *str)
{
        size_t len = 0;

        while (*str) {
                unsigned char c = (unsigned char) *str;
                if ((c & 0xC0) != 0x80)
                        len++;
                str++;
        }

        return len;
}

static void run_help(struct argparser *ap)
{
        printf("%s", argparser_help(ap));
        exit(0);
}

static void run_version(void)
{
        printf("meas version: %s\n", MEAS_VERSION);
        exit(0);
}

static int run_strlen(int is_unicode, const char *str)
{
        printf("%zu\n", is_unicode ? strlen_utf8(str) : strlen(str));
        return 0;
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *opt_help;
        struct option *opt_version;
        struct option *opt_str;
        struct option *opt_uni;

        ap = argparser_create("meas");
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &opt_help, "h", "help", "show this help message and exit", OP_NULL);
        argparser_add0(ap, &opt_version, "version", NULL, "show current version", OP_NULL);
        argparser_add1(ap, &opt_str, "s", "str", "input string value", OP_NULL | OP_REQVAL);
        argparser_add0(ap, &opt_uni, "u", "unicode", "use unicode parse string length", OP_NULL);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        if (opt_help)
                run_help(ap);

        if (opt_version)
                run_version();

        if (opt_str || argparser_count(ap) > 0) {
                if (!opt_str) {
                        /* Default execute get unicode string length */
                        run_strlen(1, argparser_val(ap, 0));
                } else {
                        run_strlen(opt_uni != NULL, opt_str->sval);
                }
        }

        argparser_free(ap);

        return 0;
}
