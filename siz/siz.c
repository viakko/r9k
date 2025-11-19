/* len.c: Created by Ekko on 2025/11/18 */
#include <stdio.h>
#include <string.h>
#include <argparse.h>

#define NSZ_VERSION "1.0.0"

static struct option options[] = {
        { 'v', "version", no_argument, opt_single, "版本号" },
        { 'u', "utf8", no_argument, opt_single, "按字符计算" },
        { 0 },
};

static size_t utf8len(const char *str)
{
        size_t len = 0;

        while (*str)
                len += (*str++ & 0xc0) != 0x80;

        return len;
}

int main(int argc, char **argv)
{

        argparse_t *ap;
        ap = argparse_parse(options, argc, argv);

        if (!ap) {
                printf("%s\n", argparse_error());
                exit(1);
        }

        if (argparse_has(ap, "version")) {
                printf("siz version: %s\n", NSZ_VERSION);
                exit(0);
        }

        if (argparse_has(ap, "utf8")) {
                printf("%ld\n", utf8len(argparse_arg(ap)));
                exit(0);
        }

        printf("%ld\n", strlen(argparse_arg(ap)));

        argparse_free(ap);

        return 0;
}
