/*
 * SPDX-License-Identifier: MIT
 * Copyright (m) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <r9k/argparse.h>
#include <r9k/string.h>
#include <r9k/panic.h>

#define BUFSIZE 262144 /* 256kb */

struct worker_arg_t
{
        const char *path;
        struct option *m;
        struct option *l;
        ssize_t ret;
        int err;
};

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str) {
                unsigned char m = (unsigned char) *str;
                str++;
                if ((m & 0xC0) != 0x80)
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

static ssize_t stream_count(FILE *fptr, struct option *m, struct option *l, int *err)
{
        char buf[BUFSIZE + 1];
        ssize_t total = 0;
        ssize_t n;

        while ((n = (ssize_t) fread(buf, 1, BUFSIZE, fptr)) > 0) {
                if (m) {
                        buf[n] = '\0';
                        total += (ssize_t) utf8len(buf);
                } else if (l) {
                       total += (ssize_t) line_count(buf, n);
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
                goto out;
        }

        arg->ret = stream_count(fp, arg->m, arg->l, &arg->err);

        fclose(fp);

out:
        return NULL;
}

static void process_stream(struct option *f,
                           struct option *m,
                           struct option *l)
{
        ssize_t total = 0;

        /* read stdin */
        if (f == NULL) {
                int err = 0;
                total = stream_count(stdin, m, l, &err);
                PANIC_IF(total < 0, "ERROR: %s\n", strerror(err));
                printf("%ld\n", total);
                return;
        }

        pthread_t threads[f->nval];
        struct worker_arg_t args[f->nval];

        /* create thread */
        for (uint32_t i = 0; i < f->nval; i++) {
                args[i].path = f->vals[i];
                args[i].m = m;
                args[i].l = l;
                args[i].ret = 0;
                args[i].err = 0;
                pthread_create(&threads[i], NULL, stream_count_worker, &args[i]);
        }

        /* run thread */
        for (uint32_t i = 0; i < f->nval; i++) {
                pthread_join(threads[i], NULL);
                if (args[i].err != 0)
                        PANIC("ERROR: %s: %s\n", args[i].path, strerror(args[i].err));
                printf("%8ld %s\n", args[i].ret, args[i].path);
                total += args[i].ret;
        }

        if (f->nval > 1)
                printf("%8ld total\n", total);
}

int main(int argc, char* argv[])
{
        struct argparse *ap;
        struct option *c, *m, *l, *f;

        ap = argparse_create("strc", "1.0");
        PANIC_IF(!ap, "argparse initialize failed");

        argparse_add0(ap, &c, "c", NULL, "count bytes.", NULL, 0);
        argparse_add0(ap, &m, "m", NULL, "count UTF-8 characters", NULL, 0);
        argparse_add0(ap, &l, "l", NULL, "count line.", NULL, 0);
        argparse_addn(ap, &f, "f", NULL, "count files.", "path", 128, NULL, O_REQUIRED);

        argparse_mutual_exclude(ap, &c, &m, &l);

        if (argparse_run(ap, argc, argv) != A_OK)
                PANIC("%s\n", argparse_error(ap));

        if (f || argparse_count(ap) == 0) {
                process_stream(f, m, l);
        } else {
                const char *str = argparse_val(ap, 0);
                if (m) {
                        printf("  %ld\n", utf8len(str));
                } else if (l) {
                        printf("  %ld\n", line_count(str, strlen(str)));
                } else {
                        printf("  %ld\n", strlen(str));
                }
        }

        argparse_free(ap);

        return 0;
}