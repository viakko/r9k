/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * argparse - Lightweight command-line argument parsing library
 *
 * Provides argument parsing with support for both
 * short and long options, required/optional arguments, and multi-value options.
 */
#include <argparse.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>

static char error_buf[4096];

struct optent
{
        const struct option *opt;
        char **vals;
        size_t nval;
        unsigned int seen;
};

struct argparse
{
        const struct option *opts;
        size_t nopt;
        struct optent *ents;
        size_t nent;
        char *arg; /* non option */
};

static void seterror(const char *fmt, ...)
{
        va_list va;
        va_start(va, fmt);
        vsnprintf(error_buf, sizeof(error_buf), fmt, va);
        va_end(va);
}

static const struct optent *find_optent(argparse_t *ap, const char *optname)
{
        size_t n;
        const struct option *opt;

        n = strlen(optname);

        for (size_t i = 0; i < ap->nent; i++) {
                opt = ap->ents[i].opt;
                if (n == 1) {
                        if (optname[0] == opt->short_name)
                                return &ap->ents[i];
                        continue;
                }

                if (strcmp(opt->long_name, optname) == 0)
                        return &ap->ents[i];
        }

        return NULL;
}

static size_t count_options(const struct option *opts)
{
        size_t n = 0;

        while (opts[n].long_name)
                n++;

        return n;
}

static ssize_t parse_short_opts(argparse_t *ap, const char *arg, size_t **p_iarr, uint32_t *p_count)
{
        char short_name;
        size_t i;
        const struct option *opt;
        size_t *iarr = NULL;
        size_t *tmp_iarr = NULL;
        uint32_t count = 0;

        while (*arg) {
                short_name = arg[0];

                for (i = 0; i < ap->nopt; i++) {
                        opt = &ap->opts[i];

                        if (opt->short_name == short_name) {
                                count += 1;

                                if (!(opt->flags & allow_group)) {
                                        seterror("option <--%s> not support group", opt->long_name);
                                        goto fail;
                                }

                                if (iarr == NULL) {
                                        iarr = calloc(1, sizeof(*iarr));
                                        if (!iarr)
                                                goto fail;
                                }

                                tmp_iarr = realloc(iarr, sizeof(*iarr) * count);
                                if (!tmp_iarr)
                                        goto fail;

                                iarr = tmp_iarr;

                                iarr[count - 1] = i;
                                break;
                        }
                }

                arg++;
        }

        *p_iarr = iarr;
        *p_count = count;

        return 0;

fail:
        if (iarr)
                free(iarr);

        return -1;
}

static ssize_t find_options(argparse_t *ap, const char *arg, size_t **p_iarr, uint32_t *p_count)
{
        ssize_t i;
        unsigned int is_short = 0;
        const struct option *op;

        if (!arg || arg[0] != '-')
                return -1;

        if (arg[0] == '-' && arg[1] != '-') {
                arg += 1;
                is_short = 1;
        } else if (arg[0] == '-' && arg[1] == '-') {
                arg += 2;
        }

        if (is_short)
                return parse_short_opts(ap, arg, p_iarr, p_count);

        for (i = 0; i < ap->nopt; i++) {
                op = &ap->opts[i];

                if (strcmp(op->long_name, arg) == 0) {
                        *p_iarr = malloc(sizeof(size_t));
                        if (!*p_iarr) {
                                seterror(strerror(errno));
                                return -1;
                        }

                        (*p_iarr)[0] = i;
                         *p_count = 1;

                        return 0;
                }
        }

        return -1;
}

static int add_option_value(argparse_t *ap, size_t optid, const char *val)
{
        size_t i;
        const struct option *op = NULL;
        struct optent *ent = NULL;
        struct optent *tmp_ents;
        char **tmp_vals;
        char *valdup;

        op = &ap->opts[optid];

        for (i = 0; i < ap->nent; i++) {
                if (ap->ents[i].opt == op) {
                        ent = &ap->ents[i];
                        break;
                }
        }

        if (ent != NULL && val != NULL && !(op->flags & opt_multi)) {
                seterror("option: --%s too many argument, unsupported multi args", op->long_name);
                return -1;
        }

        if (!ent) {
                tmp_ents = realloc(ap->ents, sizeof(*ap->ents) * (ap->nent + 1));
                if (!tmp_ents) {
                        seterror(strerror(errno));
                        return -1;
                }

                ap->ents = tmp_ents;
                ent = &ap->ents[ap->nent++];
                ent->opt = &ap->opts[optid];
                ent->vals = NULL;
                ent->nval = 0;
                ent->seen = 0;
        }

        ent->seen++;

        if (val) {
                tmp_vals = realloc(ent->vals, sizeof(*tmp_vals) * (ent->nval + 1));
                if (!tmp_vals) {
                        seterror(strerror(errno));
                        return -1;
                }

                ent->vals = tmp_vals;

                valdup = strdup(val);
                if (!valdup) {
                        seterror(strerror(errno));
                        return -1;
                }

                ent->vals[ent->nval++] = valdup;
        }

        return 0;
}

static int parse_arguments(argparse_t *ap, const struct option *opts, int *i, int argc, char **argv)
{
        size_t *iarr = NULL;
        uint32_t count;
        const struct option *op;
        char *val = NULL;
        int next = *i + 1;

        if (find_options(ap, argv[*i], &iarr, &count) == -1)
                goto fail;

        for (uint32_t j = 0; j < count; j++) {
                op = &ap->opts[iarr[j]];

                if (next < argc && !val) {
                        if ((op->flags & required_argument) && argv[next][0] != '-') {
                                val = argv[next];
                                (*i)++;
                        }
                }

                if (op->flags & required_argument && !val) {
                        seterror("option: --%s required argument", op->long_name);
                        goto fail;
                }

                if (add_option_value(ap, iarr[j], val) == -1)
                        goto fail;
        }

        free(iarr);
        iarr = NULL;
        count = 0;

        return 0;
fail:
        if (iarr)
                free(iarr);

        return -1;
}

argparse_t *argparse_parse(const struct option *opts, int argc, char **argv)
{
        argparse_t *ap;

        if (!opts || argc < 1 || !argv) {
                seterror("invalid arguments");
                return NULL;
        }

        ap = calloc(1, sizeof(*ap));
        ap->opts = opts;
        ap->nopt = count_options(opts);
        ap->arg = NULL;

        for (int i = 1; i < argc; i++) {
                if (argv[i][0] != '-') {
                        if (ap->arg) {
                                seterror("too many arguments");
                                argparse_free(ap);
                                return NULL;
                        }

                        ap->arg = strdup(argv[i]);
                        if (!ap->arg) {
                                seterror(strerror(errno));
                                argparse_free(ap);
                                return NULL;
                        }

                        continue;
                }

                if (parse_arguments(ap, opts, &i, argc, argv) == -1) {
                        argparse_free(ap);
                        return NULL;
                }
        }

        return ap;
}

void argparse_free(argparse_t *ap)
{
        struct optent *op;

        if (!ap)
                return;

        if (ap->arg)
                free(ap->arg);

        for (size_t i = 0; i < ap->nent; i++) {
                op = &ap->ents[i];

                for (size_t j = 0; j < op->nval; j++)
                        free(op->vals[j]);

                free(op->vals);
        }

        free(ap->ents);
        free(ap);
}

int argparse_has(argparse_t *ap, const char *name)
{
        return find_optent(ap, name) ? 1 : 0;
}

const char *argparse_val(argparse_t *ap, const char *name)
{
        const char **vals;

        vals = argparse_multi_val(ap, name, NULL);
        if (!vals)
                return NULL;

        return vals[0];
}

const char **argparse_multi_val(argparse_t *ap, const char *name, size_t *nval)
{
        const struct optent *ent;

        ent = find_optent(ap, name);
        if (!ent)
                return NULL;

        if (nval)
                *nval = ent->nval;

        return (const char **) ent->vals;
}

const char *argparse_arg(argparse_t *ap)
{
        return ap->arg;
}

const char *argparse_error(void)
{
        return error_buf;
}