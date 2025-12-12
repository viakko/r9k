/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <r9k/argparser.h>
#include <r9k/typedefs.h>
#include <r9k/string.h>

#define BUFSIZE 262144 /* 256kb */

struct worker_arg_t
{
        const char *path;
        struct option *ischr;
        ssize_t ret;
        int err;
};

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str) {
                unsigned char c = (unsigned char) *str;
                str++;
                if ((c & 0xC0) != 0x80)
                        len++;
        }

        return len;
}

static ssize_t stream_count(FILE *fptr, struct option *ischr, int *err)
{
        char buf[BUFSIZE + 1];
        ssize_t total = 0;
        ssize_t n;

        while ((n = fread(buf, 1, BUFSIZE, fptr)) > 0) {
                if (ischr) {
                        buf[n] = '\0';
                        total += utf8len(buf);
                } else {
                        total += n;
                }
        }

        if (ferror(fptr)) {
                *err = errno;
                return -1;
        }

        return total;
}

static void *stream_count_worker(void *_arg)
{
        struct worker_arg_t *arg = _arg;

        FILE* fp = fopen(arg->path, "r");
        if (!fp) {
                arg->err = errno;
                return NULL;
        }

        arg->ret = stream_count(fp, arg->ischr, &arg->err);

        fclose(fp);
        return NULL;
}

static void process_stream(struct option *files, struct option *ischr)
{
        size_t total = 0;

        /* read stdin */
        if (files == NULL) {
                int err = 0;
                total = stream_count(stdin, ischr, &err);
                if (total <= 0)
                        die("ERROR read in standard input: %s\n", strerror(err));
                printf("%ld\n", total);
                return;
        }

        pthread_t threads[files->nval];
        struct worker_arg_t args[files->nval];

        /* create thread */
        for (uint32_t i = 0; i < files->nval; i++) {
                args[i].path = files->vals[i];
                args[i].ischr = ischr;
                args[i].ret = 0;
                args[i].err = 0;
                pthread_create(&threads[i], NULL, stream_count_worker, &args[i]);
        }

        /* run thread */
        for (uint32_t i = 0; i < files->nval; i++) {
                pthread_join(threads[i], NULL);
                if (args[i].ret < 0)
                        die("ERROR read in file %s: %s\n", args[i].path, strerror(args[i].err));
                printf("  %ld %s\n", args[i].ret, args[i].path);
                total += args[i].ret;
        }

        if (files->nval > 1)
                printf("  %ld total\n", total);
}

int main(int argc, char* argv[])
{
        struct argparser *ap;
        struct option *help;
        struct option *version;
        struct option *ischr;
        struct option *files;

        ap = argparser_create("strl", "1.0.0");
        if (!ap)
                die("argparser initialize failed");

        argparser_add0(ap, &help, "h", "help", "show this help message.", ACB_HELP, 0);
        argparser_add0(ap, &version, "version", NULL, "show current version.", ACB_VERSION, 0);
        argparser_add0(ap, &ischr, "c", NULL, "count characters by unicode.", NULL, 0);
        argparser_addn(ap, &files, "f", NULL, 128, "count files.", NULL, OPT_REQUIRED);

        if (argparser_run(ap, argc, argv) != 0)
                die("%s", argparser_error(ap));

        if (files || argparser_count(ap) == 0) {
                process_stream(files, ischr);
        } else {
                const char *str = argparser_val(ap, 0);
                printf("  %ld\n", ischr ? utf8len(str) : strlen(str));
        }

        argparser_free(ap);

        return 0;
}