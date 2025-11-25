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

#define MEAS_VERSION "1.0.0"

int main(int argc, char **argv)
{
        struct option *help;
        struct option *std;
        struct option *a, *b, *c, *d;

        struct argparser *ap = argparser_create();

        argparser_add0(ap, &help, "h", "help", "Print this program help");
        argparser_add1(ap, &std, "std", "standard", "Uses the standard");
        argparser_add0(ap, &a, "a", "aa", "a");
        argparser_add0(ap, &b, "b", "bb", "ab");
        argparser_add0(ap, &c, "c", "cc", "abc");
        argparser_add1(ap, &d, "d", "dd", "abcd");

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                exit(1);
        }

        if (std)
                printf("std: %s\n", std->sval);

        if (d)
                printf("d: %s\n", d->sval);

        argparser_free(ap);

        return 0;
}
