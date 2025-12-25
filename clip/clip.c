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

static int print_callback(struct argparse *ap, struct option *opt)
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

        exit(0);
}

static void clipboard_write(struct argparse *ap)
{
        __attr_ignore(ap);

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
}

int main(int argc, char *argv[])
{
        struct argparse *ap;

        ap = argparse_create("clip", "1.0");
        PANIC_IF(!ap, "error: argparse initialize failed");

        argparse_add0(ap, NULL, "print", NULL, "read ontents in clipboard", print_callback, 0);
        argparse_add0(ap, NULL, "q", "quiet", "quiet write to clipboard", NULL, 0);

        if (argparse_run(ap, argc, argv) != 0)
                PANIC("error: %s\n", argparse_error(ap));

        /* default call write */
        clipboard_write(ap);

        argparse_destory(ap);

        return 0;
}
