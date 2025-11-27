/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <argparser.h>
#include <string.h>

#define MEAS_VERSION "1.0"

static size_t strlen_utf8(const char *str)
{
        size_t len = 0;

        while (*str) {
                unsigned char c = (unsigned char) *str;
                if ((c & 0xC0) != 0x80)
                        len++;
                str++;
        }

        return len;
}

static void show_version()
{
        printf("meas version: %s\n", MEAS_VERSION);
        exit(0);
}

static void show_help(void)
{
        printf(
            "Usage: meas [OPTIONS]\n"
            "Options:\n"
            "  -h, --help            show this help message and exit\n"
            "  -version, --version   show current version\n"
            "  -s, --str <value>     input string value (required)\n"
            "  -u, --unicode         use unicode parse string length\n"
        );
        exit(0);
}

static void show_strlen(struct option *unicode, const char *str)
{
        printf("%zu\n", unicode ? strlen_utf8(str) : strlen(str));
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help;
        struct option *version;
        struct option *str;
        struct option *unicode;

        ap = argparser_create();
        if (!ap) {
                fprintf(stderr, "Failed to create argparser\n");
                exit(1);
        }

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", OP_NULL);
        argparser_add0(ap, &version, "version", "version", "show current version", OP_NULL);
        argparser_add1(ap, &str, "s", "str", "input string value", OP_NULL | OP_REQVAL);
        argparser_add0(ap, &unicode, "u", "unicode", "use unicode parse string length", OP_NULL);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                exit(1);
        }

        if (help) show_help();
        if (version) show_version();
        if (str) show_strlen(unicode, str->sval);

        argparser_free(ap);

        return 0;
}
