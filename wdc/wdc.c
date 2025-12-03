/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <argparser.h>
#include <string.h>
#include <r9k/typedefs.h>
#include <errno.h>

#include <r9k/ioutils.h>

#define WDC_VERSION "1.0"

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

static size_t length(const char *str, bool _char)
{
        return _char ? utf8len(str) : strlen(str);
}

static size_t lines(const char *str)
{
        size_t count = 0;

        while (*str) {
                if (*str == '\n')
                        count++;
                str++;
        }

        return count;
}

static size_t wdc(struct argparser *ap, const char *str)
{
        if (argparser_find(ap, "l"))
                return lines(str);

        if (argparser_find(ap, "c"))
                return length(str, true);

        return length(str, false);
}

static const char *strval(struct argparser *ap, bool *p_need_free)
{
        const char *buf = NULL;

        if (argparser_count(ap) > 0) {
                buf = argparser_val(ap, 0);
                *p_need_free = false;
        } else {
                buf = readin();
                if (!buf)
                        return NULL;
                *p_need_free = true;
        }

        return buf;
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help, *version, *c, *l;

        const char *buf;
        bool need_free = false;

        ap = argparser_create("wdc", WDC_VERSION);
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", __acb_help, opt_none);
        argparser_add0(ap, &version, "version", NULL, "show current version", __acb_version, opt_none);
        argparser_add0(ap, &c, "c", NULL, "character count", NULL, opt_none | opt_nogroup);
        argparser_add0(ap, &l, "l", NULL, "line count", NULL, opt_none | opt_nogroup);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        if ((buf = strval(ap, &need_free)) == NULL) {
                fprintf(stderr, "%s\n", strerror(errno));
                return -1;
        }

        printf("%zu\n", wdc(ap, buf));

        if (need_free)
                free((void *) buf);

        argparser_free(ap);

        return 0;
}
