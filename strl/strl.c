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
        struct option *c;
        struct option *l;
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

static size_t line_count(const char *buf, size_t size)
{
        size_t count = 0;
        size_t len = 0;

        while (len < size) {
                if (buf[len] == '\n')
                        count++;
                len++;
        }

        return count;
}

static ssize_t stream_count(FILE *fptr, struct option *c, struct option *l, int *err)
{
        char buf[BUFSIZE + 1];
        ssize_t total = 0;
        ssize_t n;

        while ((n = fread(buf, 1, BUFSIZE, fptr)) > 0) {
                if (c) {
                        buf[n] = '\0';
                        total += utf8len(buf);
                } else if (l) {
                       total += line_count(buf, n);
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

        arg->ret = stream_count(fp, arg->c, arg->l, &arg->err);

        fclose(fp);
        return NULL;
}

static void process_stream(struct option *f,
                           struct option *c,
                           struct option *l)
{
        size_t total = 0;

        /* read stdin */
        if (f == NULL) {
                int err = 0;
                total = stream_count(stdin, c, l, &err);
                if (total <= 0)
                        die("ERROR read in standard input: %s\n", strerror(err));
                printf("%ld\n", total);
                return;
        }

        pthread_t threads[f->nval];
        struct worker_arg_t args[f->nval];

        /* create thread */
        for (uint32_t i = 0; i < f->nval; i++) {
                args[i].path = f->vals[i];
                args[i].c = c;
                args[i].l = l;
                args[i].ret = 0;
                args[i].err = 0;
                pthread_create(&threads[i], NULL, stream_count_worker, &args[i]);
        }

        /* run thread */
        for (uint32_t i = 0; i < f->nval; i++) {
                pthread_join(threads[i], NULL);
                if (args[i].ret < 0)
                        die("ERROR read in file %s: %s\n", args[i].path, strerror(args[i].err));
                printf("  %ld %s\n", args[i].ret, args[i].path);
                total += args[i].ret;
        }

        if (f->nval > 1)
                printf("  %ld total\n", total);
}

int main(int argc, char* argv[])
{
        struct argparser *ap;
        struct option *h, *v, *c, *l, *f;

        ap = argparser_create("strl", "1.0.0");
        if (!ap)
                die("argparser initialize failed");

        argparser_add0(ap, &h, "h", "help", "show this help message.", ACB_EXIT_HELP, 0);
        argparser_add0(ap, &v, "version", NULL, "show current version.", ACB_EXIT_VERSION, 0);
        argparser_add0(ap, &c, "c", NULL, "count characters by unicode.", NULL, 0);
        argparser_add0(ap, &l, "l", NULL, "count line.", NULL, 0);
        argparser_addn(ap, &f, "f", NULL, 128, "count files.", NULL, O_REQUIRED);

        argparser_mutual_exclude(ap, &c, &l);

        if (argparser_run(ap, argc, argv) != 0)
                die("%s\n", argparser_error(ap));

        if (f || argparser_count(ap) == 0) {
                process_stream(f, c, l);
        } else {
                const char *str = argparser_val(ap, 0);
                if (c) {
                        printf("  %ld\n", utf8len(str));
                } else if (l) {
                        printf("  %ld\n", line_count(str, strlen(str)));
                } else {
                        printf("  %ld\n", strlen(str));
                }
        }

        argparser_free(ap);

        return 0;
}