/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
//r9k
#include <r9k/argparser.h>
#include <r9k/typedefs.h>
#include <r9k/ioutils.h>

#include "charc.h"

#define COUNT_VERSION "1.0"

struct option *opt_chars;
struct option *opt_lines;
struct option *opt_files;

static size_t count_units(const char *text)
{
        return opt_lines ? linec(text) : charc(text, opt_chars != NULL);
}

static void count_file(const char *filename)
{
        size_t n, r = 0;
        char buf[1024 * 16];

        FILE *fp = fopen(filename, "r");
        if (!fp)
                die("Failed to open file '%s'\n", filename);

        while ((n = fread(buf, 1, sizeof(buf) - 1, fp)) > 0) {
                buf[n] = '\0';
                r += count_units(buf);
        }

        fclose(fp);
        printf("%zu\n", r);
}

static void count_stdin()
{
        size_t n, r = 0;
        char buf[1024 * 16];

        while ((n = fread(buf, 1, sizeof(buf) - 1, stdin)) > 0) {
                buf[n] = '\0';
                r += count_units(buf);
        }

        printf("%zu\n", r);
}

static void count_input(const char *text)
{
        printf("%zu\n", count_units(text));
}

static void process(struct argparser *ap)
{
        if (opt_files) {
                count_file(opt_files->sval);
                return;
        }

        if (argparser_count(ap) > 0) {
                count_input(argparser_val(ap, 0));
                return;
        }

        count_stdin();
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *opt_help;
        struct option *opt_version;

        ap = argparser_create("count", COUNT_VERSION);
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &opt_help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &opt_version, "version", NULL, "show current version", __acb_version, opt_none);
        argparser_add0(ap, &opt_chars, "c", NULL, "character count", NULL, opt_none);
        argparser_add0(ap, &opt_lines, "l", NULL, "line count", NULL, opt_none);
        argparser_add1(ap, &opt_files, "f", NULL, "read file contents", NULL, opt_reqval);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        process(ap);

        argparser_free(ap);

        return 0;
}
