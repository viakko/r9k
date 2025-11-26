/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * argparse - Lightweight command-line argument parsing library
 *
 * Provides multi-sytle command parsing with support for both
 * short and long options. Short option support string type,
 * and single-character options can be grouped.
 *
 * The rules:
 *  - If a long option include 'abc', it gets proirity in processing.
 *  - If 'abc' not found in long option, it will be split into single-character
 *    options for short option matching.
 *  - Supports argument specified with either spaces or equal signs.
 *  - When arguments are specified using spaces, multiple values are supported.
 */
#include <stdio.h>
#include <stdlib.h>
#include <argparser.h>
#include <string.h>

#define MEAS_VERSION "1.0.0"

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help;
        struct option *str;
        struct option *unicode;

        ap = argparser_create();
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", OP_NULL);
        argparser_add1(ap, &str, "S", "str", "input string value", OP_NULL | OP_REQVAL);
        argparser_add0(ap, &unicode, "u", "unicode", "input string value", OP_NULL);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        if (help) {
                printf("Usage [-h] [-u] [-S]\n");
                goto end;
        }

        if (str) {
                printf("%lu\n", strlen(str->sval));
                goto end;
        }

end:
        argparser_free(ap);

        return 0;
}
