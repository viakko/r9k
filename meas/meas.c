/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * siz - A command-line tool for calculation file sizes and string lengths
 *
 * Supports file size calculation (with human-readable output)
 * and string length counting (both byte length and UTF-8 character count)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argparser.h>

#define MEAS_VERSION "1.0.0"

int main(int argc, char **argv)
{
        struct optunit *help;
        struct optunit *files;
        struct optunit *std;

        struct argparser *ap = argparser_create();

        argparser_add0(ap, &help, "h", "help", "Print this program help");
        argparser_add1(ap, &files, "std", "standard", "Uses the standard");
        argparser_add1(ap, &std, "f", "files", "Multiple files");

        if (argparser_run(ap, argc, argv) != 0)
                exit(1);

        if (std) {
                printf("%s\n", std->sval);
        }

        if (files) {
                for (int i = 0; i < files->count; i++)
                        printf("%s\n", files->vals[i]);
        }

        argparser_free(ap);

        return 0;
}
