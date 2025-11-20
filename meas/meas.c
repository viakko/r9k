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
#include <string.h>
#include <errno.h>
#include <argparse.h>

#define SIZ_VERSION "1.0.0"

static struct option options[] = {
        { 'v', "version", 0, "版本号" },
        { 'u', "utf8", 0, "按字符计算" },
        { '?', "raw", 0, "以字节数显示" },
        { 'f', "file", 0, "计算文件大小" },
        { 0 },
};

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str)
                len += (*str++ & 0xc0) != 0x80;

        return len;
}

static ssize_t filesize(const char *path)
{
        FILE *fp;
        long int size;

        fp = fopen(path, "rb");
        if (!fp)
                return -1;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        fclose(fp);

        return size;
}

static double human_size(size_t size, const char **unit)
{
        /* K Byte */
        if (size <= (1024 * 1024)) {
                *unit = "K";
                return (double) size / 1024;
        }

        /* M Byte */
        if (size <= (1024 * 1024 * 1024)) {
                *unit = "M";
                return (double) size / 1024 / 1024;
        }

        /* G Byte */
        *unit = "G";
        return (double) size / 1024 / 1024 / 1024;
}

int main(int argc, char **argv)
{
        const char *arg;
        argparse_t *ap;
        ap = argparse_parse(options, argc, argv);

        if (!ap) {
                fprintf(stderr, "siz error: %s\n", argparse_error());
                exit(1);
        }

        if (argparse_has(ap, "version")) {
                printf("siz version: %s\n", SIZ_VERSION);
                exit(0);
        }

        arg = argparse_arg(ap);
        if (!arg) {
                fprintf(stderr, "siz error: invalid argument\n");
                exit(1);
        }

        if (argparse_has(ap, "file")) {
                double hsize;
                const char *unit;
                ssize_t size = filesize(arg);

                if (size == -1) {
                        fprintf(stderr, "siz error: %s\n", strerror(errno));
                        exit(1);
                }

                if (argparse_has(ap, "raw")) {
                        printf("%ld\n", size);
                        exit(0);
                }

                hsize = human_size(size, &unit);
                printf("%.2f%s\n", hsize, unit);

                exit(0);
        }

        if (argparse_has(ap, "utf8")) {
                printf("%ld\n", utf8len(arg));
                exit(0);
        }

        printf("%ld\n", strlen(arg));

        argparse_free(ap);

        return 0;
}
