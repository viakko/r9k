/*
 * SPDX-License-Identifier: MIT
 * Copyright (m) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <r9k/argparser.h>
#include <r9k/string.h>
#include <r9k/panic.h>
#include <r9k/compiler_attrs.h>

static int hex_val(char c)
{
        if ('0' <= c && c <= '9') return c - '0';
        if ('a' <= c && c <= 'f') return c - 'a' + 10;
        if ('A' <= c && c <= 'F') return c - 'A' + 10;
        return -1;
}

static int is_unreserved(unsigned char c)
{
        if (isalnum(c))
                return 1;

        switch (c) {
                case '-':
                case '_':
                case '.':
                case '~':
                        return 1;
                default:
                        return 0;
        }
}

static int url_encode(struct argparser *ap, struct option *o_encode)
{
        __attr_ignore(ap);

        const unsigned char *p;
        char *out, *q;
        size_t len = 0;

        for (p = (const unsigned char *) o_encode->sval; *p; p++)
                len += is_unreserved(*p) ? 1 : 3;

        out = malloc(len + 1);
        if (!out)
                return -1;

        q = out;
        for (p = (const unsigned char *) o_encode->sval; *p; p++) {
                if (is_unreserved(*p)) {
                        *q++ = *p;
                } else {
                        sprintf(q, "%%%02X", *p);
                        q += 3;
                }
        }

        *q = '\0';

        if (!argparser_has(ap, "no-title"))
                printf("=== ENCODING ===\n");
        printf("%s\n", out);

        free(out);

        return 0;
}

static int url_decode(struct argparser *ap, struct option *o_decode)
{
        __attr_ignore(ap);

        char *out, *q;
        const char *p, *s;

        s = o_decode->sval;

        out = malloc(strlen(s) + 1);
        if (!out)
                return -1;

        q = out;
        for (p = s; *p; p++) {
                if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
                        int hi = hex_val(p[1]);
                        int lo = hex_val(p[2]);
                        *q++ = (char)((hi << 4) | lo);
                        p += 2;
                } else {
                        *q++ = *p;
                }
        }
        *q = '\0';

        if (!argparser_has(ap, "no-title"))
                printf("=== DECODING ===\n");
        printf("%s\n", out);

        free(out);

        return 0;
}

int main(int argc, char* argv[])
{
        struct argparser *ap;
        struct option *encode;
        struct option *decode;
        struct option *no_title;

        ap = argparser_create("url", "1.0");
        PANIC_IF(!ap, "argparser initialize failed");

        argparser_add1(ap, &encode, NULL, "encode", "url encoding", url_encode, O_REQUIRED);
        argparser_add1(ap, &decode, NULL, "decode", "url decoding", url_decode, O_REQUIRED);
        argparser_add0(ap, &no_title, NULL, "no-title", "do not show option title", NULL, 0);

        if (argparser_run(ap, argc, argv) != 0)
                PANIC("%s\n", argparser_error(ap));

        argparser_free(ap);

        return 0;
}