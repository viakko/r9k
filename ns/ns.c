/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * Network debugger
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <errno.h>
#include <argparser.h>
#include <sys/ioctl.h>
#include <fx/typedefs.h>

/* --flushdns */
static int on_flush(struct argparser *ap, struct option *opt)
{
        int r;

#ifdef _WIN32
        r = system("ipconfig /flushdns");
#elif __linux__
        r = system("sudo systemd-resolve --flush-caches");
#elif __APPLE__
        r = system("sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder 2>/dev/null");
        if (r != 0)
                r = system("sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder 2>/dev/null");
        if (r != 0)
                r = system("sudo killall -HUP mDNSResponder 2>/dev/null");
#endif
        if (r == 0)
                printf("DNS cache flushed successfully.\n");
        else
                printf("Failed to flush DNS cache.\n");

        exit(0);
}

static int on_dns(struct argparser *ap, struct option *opt)
{
        struct __res_state res;

        if (res_ninit(&res) != 0)
                die("res_ninit failed, %s", strerror(errno));

        for (int i = 0; i < res.nscount; i++)
                printf("%s\n", inet_ntoa(res.nsaddr_list[i].sin_addr));

        res_nclose(&res);

        exit(0);
}

static int on_resolv(struct argparser *ap, struct option *opt)
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

static int on_iface(struct argparser *ap, struct option *opt)
{
        struct ifaddrs *ifaddr, *ifa;

        if (getifaddrs(&ifaddr) == -1) {
                perror("getifaddrs");
                exit(1);
        }

        for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr)
                        continue;

                if (ifa->ifa_addr->sa_family == AF_INET) {
                        char ip[INET_ADDRSTRLEN];
                        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                        inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
                        printf("%s\t%s\n", ifa->ifa_name, ip);
                } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                        char ip[INET6_ADDRSTRLEN];
                        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)ifa->ifa_addr;
                        inet_ntop(AF_INET6, &sa->sin6_addr, ip, sizeof(ip));
                        printf("%s\t%s\n", ifa->ifa_name, ip);
                }
        }

        freeifaddrs(ifaddr);
        exit(0);
}

int main(int argc, char **argv)
{
        struct argparser *ap;
        struct option *opt_dns;
        struct option *opt_resolv;
        struct option *opt_flush_dns;
        struct option *opt_iface;
        struct option *opt_verbose;
        struct option *opt_help;

        ap = argparser_create("ns");
        if (!ap)
                return -1;

        argparser_add1(ap, &opt_verbose, "v", "verbose", "show more information", NULL, opt_none);
        argparser_add1(ap, &opt_dns, "dns", NULL, "show resolv DNS address and exit", on_dns, opt_none);
        argparser_add0(ap, &opt_resolv, "resolv", NULL, "show resolv.conf and exit", on_resolv, opt_none);
        argparser_add0(ap, &opt_flush_dns, NULL, "flushdns", "flush system DNS cache and exit", on_flush, opt_none);
        argparser_add0(ap, &opt_iface, "i", "interface", "list interface address", on_iface, opt_none);
        argparser_add0(ap, &opt_help, "h", "help", "show this help message and exit", NULL, opt_none);

        if (argparser_run(ap, argc, argv) != 0) {
                fprintf(stderr, "%s\n", argparser_error(ap));
                argparser_free(ap);
                return -1;
        }

        if (opt_help) {
                printf("%s", argparser_help(ap));
                argparser_free(ap);
                exit(0);
        }

        return 0;
}