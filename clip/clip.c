#include "argparser.h"
/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <r9k/ioutils.h>

#include "r9k/typedefs.h"

static void write_to_clip(const char *text)
{
        if (!text || strlen(text) == 0)
                return;

        FILE* pipe = popen("pbcopy", "w");
        if (!pipe) {
                perror("popen failed");
                return;
        }

        size_t len = strlen(text);
        size_t written = fwrite(text, 1, len, pipe);

        if (written != len) {
                perror("fwrite failed");
                pclose(pipe);
                return;
        }

        pclose(pipe);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help, *version;
        struct option *cat;

        ap = argparser_create("clip", "1.0");
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &version, "version", NULL, "show version and exit", __acb_version, opt_none);
        argparser_add0(ap, &cat, "cat", NULL, "copy to clip and cat content", NULL, opt_none);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        char *buf = readin();
        if (buf) {
                write_to_clip(buf);
                if (cat)
                        printf("%s", buf);
                free(buf);
        }

        argparser_free(ap);

        return 0;
}