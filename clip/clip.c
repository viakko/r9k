/*
 * SPDX-License-Identifier: MIT
 * Copyright (m) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <r9k/argparse.h>
#include <r9k/panic.h>
#include <r9k/ioutils.h>
#include <r9k/string.h>

static int read_callback(struct argparse *ap, struct option *opt)
{
        __attr_ignore2(ap, opt);

        FILE *fp = popen("pbpaste", "r");
        if (!fp)
                PANIC("error: popen() failed\n");

        char *buf = readall(fp);
        if (buf) {
                printf("%s", buf);
                free(buf);
        }

        pclose(fp);

        return 0;
}

static int write_callback(struct argparse *ap, struct option *opt)
{
        __attr_ignore2(ap, opt);

        FILE *fp = popen("pbcopy", "w");
        if (!fp)
                PANIC("error: popen() failed\n");

        char *buf = readall(stdin);
        if (!buf)
                PANIC("error: realall() failed\n");

        if (!argparse_has(ap, "q"))
                printf("%s", buf);

        fwrite(buf, 1, strlen(buf), fp);

        free(buf);
        pclose(fp);

        return 0;
}

int main(int argc, char *argv[])
{
        struct argparse *ap;
        struct option *opt_read = NULL;
        struct option *opt_write = NULL;
        struct option *opt_quiet = NULL;

        ap = argparse_create("clip", "1.0");
        PANIC_IF(!ap, "argparse initialize failed");

        argparse_add0(ap, &opt_read, "read", NULL, "read ontents in clipboard", read_callback, 0);
        argparse_add0(ap, &opt_write, "write", NULL, "write ontents to clipboard", write_callback, 0);
        argparse_add0(ap, &opt_quiet, "q", "quiet", "quiet write to clipboard", NULL, 0);

        if (argparse_run(ap, argc, argv) != A_OK)
                PANIC("%s\n", argparse_error(ap));

        /* default call write */
        if (!opt_read && !opt_write)
                write_callback(ap, opt_write);

        argparse_free(ap);

        return 0;
}
