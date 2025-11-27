/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * Network debugger
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <argparser.h>
#include <fx/ioutils.h>

/* --flushdns */
static void handle_flushdns()
{
        int ret = -1;

#ifdef _WIN32
        system("ipconfig /flushdns");
#elif __linux__
        system("sudo systemd-resolve --flush-caches");
#elif __APPLE__
        // try macOS 13 (Ventura)
        ret = system("sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder 2>/dev/null");

        if (ret != 0) {
                // try macOS 12 (Monterey)
                ret = system("sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder 2>/dev/null");
        }

        if (ret != 0) {
                // try macOS 11 (Big Sur)
                ret = system("sudo killall -HUP mDNSResponder 2>/dev/null");
        }
#endif

        if (ret == 0) {
                printf("DNS cache flushed successfully.\n");
        } else {
                printf("Failed to flush DNS cache.\n");
        }
}

static void show_dns_list()
{
        struct __res_state res;

        if (res_ninit(&res) != 0) {
                perror("res_ninit failed");
                exit(1);
        }

        for (int i = 0; i < res.nscount; i++)
                printf("%s\n", inet_ntoa(res.nsaddr_list[i].sin_addr));

        res_nclose(&res);
        exit(0);
}

static void show_resolv_conf()
{
        char *buf = freadbuf("/etc/resolv.conf");
        if (!buf)
                exit(1);

        printf("%s", buf);
        free(buf);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *dns;
        struct option *resolv;
        struct option *flushdns;

        ap = argparser_create();
        if (!ap)
                return -1;

        argparser_add1(ap, &dns, "dns", NULL, "show resolv DNS address and exit", OP_NULL);
        argparser_add0(ap, &resolv, "resolv", NULL, "show resolv.conf and exit", OP_NULL);
        argparser_add0(ap, &flushdns, NULL, "flushdns", "flush system DNS cache and exit", OP_NULL);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        if (dns) show_dns_list();
        if (resolv) show_resolv_conf();
        if (flushdns) handle_flushdns();

        return 0;
}
