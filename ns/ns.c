/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
//r9k
#include <r9k/argparser.h>
#include <r9k/attributes.h>

#define NS_VERSION "1.0"

static int on_dns(__maybe_unused struct argparser *ap, __maybe_unused struct option *opt)
{
        char buf[4096];
        FILE *fp;

        fp = fopen("/etc/resolv.conf", "r");
        if (!fp) {
                perror("cannot open /etc/resolv.conf");
                exit(1);
        }

        while (fgets(buf, sizeof(buf), fp))
                fputs(buf, stdout);

        fclose(fp);
        exit(0);
}

static int on_flush_dns(__maybe_unused struct argparser *ap, __maybe_unused struct option *opt)
{
        int ret = 0;

        ret |= system("dscacheutil -flushcache >/dev/null 2>&1");
        ret |= system("killall -HUP mDNSResponder >/dev/null 2>&1");

        // optional macOS 12+ process, failure is non-critical.
        ret |= system("killall mDNSResponderHelper >/dev/null 2>&1");
        ret |= system("killall -HUP mDNSResponderHelper >/dev/null 2>&1");

        if (ret == 0) {
                puts("DNS cache flushed.");
                return 0;
        }

        fprintf(stderr, "Failed to flush DNS cache (need sudo?)");
        return 1;
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *help, *version;
        struct option *dns;
        struct option *flushdns;

        ap = argparser_create("ns", NS_VERSION);
        if (!ap)
                return -1;

        argparser_add0(ap, &help, "h", "help", "show this help message and exit", ACB_HELP, 0);
        argparser_add0(ap, &version, "version", NULL, "show version", ACB_VERSION, 0);
        argparser_add1(ap, &dns, "dns", NULL, "show resolv DNS address and exit", on_dns, 0);
        argparser_add1(ap, &flushdns, "flushdns", NULL, "flush DNS caches", on_flush_dns, 0);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        argparser_free(ap);

        return 0;
}