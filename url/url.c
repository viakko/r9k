/*
 * SPDX-License-Identifier: MIT
 * Copyright (m) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <r9k/argparse.h>
#include <r9k/string.h>
#include <r9k/panic.h>
#include <r9k/compiler_attrs.h>

static struct option *no_pretty;

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

static int url_encode(struct argparse *ap)
{
        __attr_ignore(ap);

        const unsigned char *p;
        char *out, *q;
        size_t len = 0;
        const char *url;

        url = argparse_val(ap, 0);
        PANIC_IF(!url, "encode: no url arguments\n");

        for (p = (const unsigned char *) url; *p; p++)
                len += is_unreserved(*p) ? 1 : 3;

        out = malloc(len + 1);
        if (!out)
                return -1;

        q = out;
        for (p = (const unsigned char *) url; *p; p++) {
                if (is_unreserved(*p)) {
                        *q++ = (char) *p;
                } else {
                        sprintf(q, "%%%02X", *p);
                        q += 3;
                }
        }

        *q = '\0';

        if (!no_pretty)
                printf("=== ENCODING ===\n");
        printf("%s\n", out);

        free(out);

        return 0;
}

static int url_decode(struct argparse *ap)
{
        __attr_ignore(ap);

        char *out, *q;
        const char *p, *url;

        url = argparse_val(ap, 0);
        PANIC_IF(!url, "decode: no url arguments\n");

        out = malloc(strlen(url) + 1);
        if (!out)
                return -1;

        q = out;
        for (p = url; *p; p++) {
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

        if (!no_pretty)
                printf("=== DECODING ===\n");
        printf("%s\n", out);

        free(out);

        return 0;
}

static int url_query(struct argparse *ap)
{
        const char *url;
        const char *q;
        const char *end;

        __attr_ignore(ap);

        url = argparse_val(ap, 0);
        PANIC_IF(!url, "qs: no url arguments\n");

        q = strchr(url, '?');
        if (!q || !*(q + 1))
                return 0;

        q++; /* skip '?' */

        if (!no_pretty)
                printf("=== QUERY ===\n");

        while (*q) {
                end = strchr(q, '&');
                if (!end)
                        end = q + strlen(q);

                if (end > q) {
                        if (!no_pretty)
                                putchar(' ');
                        fwrite(q, 1, end - q, stdout);
                        putchar('\n');
                }

                if (!*end)
                        break;

                q = end + 1;
        }

        return 0;
}

int main(int argc, char* argv[])
{
        struct argparse *ap;

        ap = argparse_create("url", "1.0");
        PANIC_IF(!ap, "argparse initialize failed");

        argparse_cmd(ap, "encode", "encode url", NULL, url_encode);
        argparse_cmd(ap, "decode", "decode url", NULL, url_decode);
        argparse_cmd(ap, "qs", "parsing query parmaeters in url", NULL, url_query);

        /* global option */
        argparse_add0(ap, &no_pretty, NULL, "no-pretty", "not format", NULL, 0);

        if (argparse_run(ap, argc, argv) != 0)
                PANIC("%s\n", argparse_error(ap));

        argparse_free(ap);

        return 0;
}