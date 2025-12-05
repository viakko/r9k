/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include "clip.h"
//std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
//r9k
#include <r9k/typedefs.h>
#include <r9k/argparser.h>
#include <r9k/ioutils.h>

static void copy_to_clipboard(struct option *opt_quiet)
{
        char *inbuf;

        inbuf = readin();
        if (!inbuf)
                die("Failed to read from stdin: %s\n", strerror(errno));

        if (clip_write(inbuf) != 0)
                die("Failed to write to clipboard: %s\n", strerror(errno));

        if (!opt_quiet)
                printf("%s", inbuf);

        free(inbuf);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *opt_help;
        struct option *opt_version;
        struct option *opt_quiet;
        struct option *opt_print;

        ap = argparser_create("clip", "1.0");
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &opt_help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &opt_version, "version", NULL, "show version and exit", __acb_version, opt_none);
        argparser_add0(ap, &opt_quiet, NULL, "quiet", "copy and not print content", NULL, opt_none);
        argparser_add0(ap, &opt_print, NULL, "print", "print clipboard content to print content", NULL, opt_none);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        if (opt_print) {
                char *buf = clip_read();
                if (!buf)
                        die("Failed to read clipboard content: %s", strerror(errno));
                printf("%s\n", buf);
                free(buf);
                argparser_free(ap);
                return 0;
        }

        copy_to_clipboard(opt_quiet);
        argparser_free(ap);

        return 0;
}