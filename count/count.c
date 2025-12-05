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

struct option *c;
struct option *l;
struct option *f;

static size_t count(const char *text)
{
        return l ? linec(text) : charc(text, c != NULL);
}

static void do_file()
{
        size_t n, r = 0;
        char buf[1024 * 16];

        FILE *fp = fopen(f->sval, "r");
        if (!fp)
                die("Failed to open file '%s'\n", f->sval);

        while ((n = fread(buf, 1, sizeof(buf) - 1, fp)) > 0) {
                buf[n] = '\0';
                r += count(buf);
        }

        fclose(fp);
        printf("%zu\n", r);
}

static void do_stdin()
{
        size_t n, r = 0;
        char buf[1024 * 16];

        while ((n = fread(buf, 1, sizeof(buf) - 1, stdin)) > 0) {
                buf[n] = '\0';
                r += count(buf);
        }

        printf("%zu\n", r);
}

static void do_count(struct argparser *ap)
{
        if (f) {
                do_file();
                return;
        }

        if (argparser_count(ap) > 0) {
                printf("%zu\n", count(argparser_val(ap, 0)));
                return;
        }

        do_stdin();
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help, *version;

        ap = argparser_create("count", COUNT_VERSION);
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &version, "version", NULL, "show current version", __acb_version, opt_none);
        argparser_add0(ap, &c, "c", NULL, "character count", NULL, opt_none);
        argparser_add0(ap, &l, "l", NULL, "line count", NULL, opt_none);
        argparser_add1(ap, &f, "f", NULL, "read file contents", NULL, opt_reqval);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        do_count(ap);

        argparser_free(ap);

        return 0;
}
