/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * argparse - Lightweight command-line argument parsing library
 *
 * Provides multi-sytle command parsing with support for both
 * short and long options. Short option support string type,
 * and single-character options can be grouped.
 *
 * The rules:
 *  - If a long option or a short option string includes 'abc', it gets priority
 *    in processing.
 *  - If 'abc' not found in long option, it will be split into single-character
 *    options for short option matching.
 *  - Supports argument specified with either spaces or equal signs.
 *  - When arguments are specified using spaces, multiple values are supported.
 *  - If a value starts with '-', you need to use equal sign accept the value.
 *  - Short option groups not support using the equal sign to accept value.
 */
#include <argparser.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_UNIT 256
#define MAX_VAL  32
#define LONG     1
#define SHORT    0

#define OPT_PREFIX(is_long) (is_long ? "--" : "-")

struct argparser
{
        const char *name;

        /* options */
        struct option *opts[MAX_UNIT];
        uint32_t nopt;

        /* position value */
        const char *vals[MAX_VAL];
        uint32_t nval;

        /* buff */
        char error[4096];
        char help[4096];
};

static void error(struct argparser *ap, const char *fmt, ...)
{
        va_list va;
        va_start(va, fmt);
        vsnprintf(ap->error, sizeof(ap->error), fmt, va);
        va_end(va);
}

static int add_option(struct argparser *ap, struct option *opt)
{
        if (ap->nopt >= MAX_UNIT) {
                error(ap, "option out of %d", MAX_UNIT);
                return -EOVERFLOW;
        }

        ap->opts[ap->nopt++] = opt;

        return 0;
}

static int store_position_val(struct argparser *ap, const char *val)
{
        if (ap->nval >= MAX_VAL) {
                error(ap, "position value out of %d", MAX_VAL);
                return -EOVERFLOW;
        }

        ap->vals[ap->nval++] = val;

        return 0;
}

static int store_option_val(struct argparser *ap, struct option *opt, int is_long, char *tok, const char *val)
{
        if (opt->count > opt->max) {
                error(ap, "%s%s option value out of %d", OPT_PREFIX(is_long), tok, opt->max);
                return -EOVERFLOW;
        }

        if (!opt->vals) {
                opt->_capacity = 16;
                opt->count = 0;
                opt->vals = calloc(opt->_capacity, sizeof(char *));
                if (!opt->vals) {
                        error(ap, strerror(errno));
                        return -ENOMEM;
                }
        }

        if (opt->count >= opt->_capacity) {
                const char **tmp_vals;
                opt->_capacity *= 2;
                tmp_vals = realloc(opt->vals, sizeof(char *) * opt->_capacity);
                if (!tmp_vals) {
                        error(ap, strerror(errno));
                        return -ENOMEM;
                }

                opt->vals = tmp_vals;
        }

        opt->vals[opt->count++] = val;

        if (opt->count == 1)
                opt->sval = opt->vals[0];

        return 0;
}

static struct option *lookup_short_char(struct argparser *ap, const char shortopt)
{
        struct option *opt;

        for (int i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (opt->shortopt) {
                        if (strlen(opt->shortopt) == 1 && shortopt == opt->shortopt[0])
                                return opt;
                }
        }

        return NULL;
}

static struct option *lookup_short_str(struct argparser *ap, const char *shortopt)
{
        struct option *opt;

        for (int i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (opt->shortopt && strcmp(shortopt, opt->shortopt) == 0)
                        return opt;
        }

        return NULL;
}

static struct option *lookup_long(struct argparser *ap, const char *longopt)
{
        struct option *opt;

        for (int i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (opt->longopt && strcmp(longopt, opt->longopt) == 0)
                        return opt;
        }

        return NULL;
}

static int take_val(struct argparser *ap, struct option *opt, int is_long, char *tok, char *dval, int *i, char *argv[])
{
        char *val = NULL;
        if (dval)
                val = dval;

        if (opt->_refs)
                *opt->_refs = opt;

        if (opt->max <= 0 && val) {
                error(ap, "option %s%s does not accept a value: '%s'", OPT_PREFIX(is_long), tok, val);
                return -EINVAL;
        }

        if (opt->max > 0) {
                if (!val) {
                        val = argv[*i + 1];

                        if (opt->flags & OP_REQVAL && !val) {
                                error(ap, "option %s%s missing required argument", OPT_PREFIX(is_long), tok);
                                return -EINVAL;
                        }

                        if (val && val[0] == '-')
                                return 0;
                        *i += 1;
                }

                store_option_val(ap, opt, is_long, tok, val);
                return 1;
        }

        return 0;
}

static int __handle_short_concat(struct argparser *ap, char *tok, int *i, char *argv[]) // NOLINT(*-reserved-identifier)
{
        char *dval = NULL;
        struct option *opt;
        char tmp[2];
        size_t len;

        len = strlen(tok);
        
        tmp[0] = tok[0];
        tmp[1] = '\0';
        opt = lookup_short_str(ap, tmp);
        if (opt != NULL && (opt->flags & OP_CONCAT)) {
                if (len > 1)
                        dval = tok + 1;

                int r = take_val(ap, opt, SHORT, tmp, dval, i, argv);
                if (r < 0)
                        return r;

                return 1;
        }

        return 0;
}

static int __handle_short_assign(struct argparser *ap, char *tok, int *i, char *argv[]) // NOLINT(*-reserved-identifier)
{
        char *dval = NULL;
        struct option *opt;

        char *eq = strchr(tok, '=');
        if (eq) {
                dval = eq + 1;
                *eq = '\0';
        }

        opt = lookup_short_str(ap, tok);
        if (opt != NULL) {
                int r = take_val(ap, opt, SHORT, tok, dval, i, argv);
                return r < 0 ? r : 1;

        }

        if (dval) {
                error(ap, "unknown option: -%s", tok);
                return -EINVAL;
        }

        return 0;
}

static int __handle_short_group(struct argparser *ap, char *tok, int *i, char *argv[]) // NOLINT(*-reserved-identifier)
{
        int has_val = 0;
        struct option *opt;
        char tmp[2];

        for (int k = 0; tok[k]; k++) {
                opt = lookup_short_char(ap, tok[k]);
                if (!opt) {
                        error(ap, "unknown option: -%c", tok[k]);
                        return -EINVAL;
                }

                if (opt->flags & OP_CONCAT) {
                        error(ap, "invalid option -%c cannot be in a group", tok[k]);
                        return -EINVAL;
                }

                if (opt->flags & OP_NOGRP) {
                        error(ap, "option -%c cannot be used as a group", tok[k]);
                        return -EINVAL;
                }

                if (has_val && opt->max > 0) {
                        error(ap, "option does not accept a value: -%c", tok[k]);
                        return -EINVAL;
                }

                tmp[0] = tok[k];
                tmp[1] = '\0';

                int r = take_val(ap, opt, SHORT, tmp, NULL, i, argv);
                if (r < 0)
                        return r;

                if (r > 0)
                        has_val = 1;
        }

        return 0;
}

static int handle_short(struct argparser *ap, int *i, char *tok, char *argv[])
{
        int r;

        /* check OP_CONCAT flag */
        r = __handle_short_concat(ap, tok, i, argv);
        if (r > 0)
                return 0;

        if (r < 0)
                return r;
        
        /* check equal sign */
        r = __handle_short_assign(ap, tok, i, argv);
        if (r > 0)
                return 0;

        if (r < 0)
                return r;
        
        return __handle_short_group(ap, tok, i, argv);
}

static int handle_long(struct argparser *ap, int *i, char *tok, char *argv[])
{
        int r;
        char *dval = NULL;
        struct option *opt;

        char *eq = strchr(tok, '=');
        if (eq) {
                dval = eq + 1;
                *eq = '\0';
        }

        opt = lookup_long(ap, tok);
        if (!opt) {
                error(ap, "unknown option: --%s", tok);
                return -EINVAL;
        }

        r = take_val(ap, opt, LONG, tok, dval, i, argv);
        return r < 0 ? r : 0;
}

struct argparser *argparser_create(const char *name)
{
        struct argparser *ap;

        ap = calloc(1, sizeof(*ap));
        if (!ap)
                return NULL;

        ap->name = name;

        return ap;
}

void argparser_free(struct argparser *ap)
{
        if (!ap)
                return;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                struct option *opt = ap->opts[i];
                if (opt->vals != NULL)
                        free(opt->vals);

                free(ap->opts[i]);
        }

        free(ap);
}

int argparser_add0(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   uint32_t flags)
{
        return argparser_addn(ap, pp_option, shortopt, longopt, 0, tips, flags);
}

int argparser_add1(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   uint32_t flags)
{
        return argparser_addn(ap, pp_option, shortopt, longopt, 1, tips, flags);
}

int argparser_addn(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   uint32_t flags)
{
        int r;
        struct option *opt;

        *pp_option = NULL;

        opt = calloc(1, sizeof(*opt));
        if (!opt)
                return -ENOMEM;

        opt->shortopt = shortopt;
        opt->longopt = longopt;
        opt->max = max;
        opt->tips = tips;
        opt->flags = flags;
        opt->_refs = pp_option;

        r = add_option(ap, opt);
        if (r != 0) {
                free(opt);
                return r;
        }

        return r;
}

int argparser_run(struct argparser *ap, int argc, char *argv[])
{
        int r;
        char *tok;

        if (!ap)
                return -EFAULT;

        for (int i = 1; i < argc; i++) {
                tok = argv[i];

                if (tok[0] != '-') {
                        store_position_val(ap, tok);
                        continue;
                }

                if (tok[1] == '-') {
                        tok += 2;

                        r = handle_long(ap, &i, tok, argv);
                        if (r != 0)
                                return r;

                        continue;
                }

                tok++;
                r = handle_short(ap, &i, tok, argv);
                if (r != 0)
                        return r;
        }

        return 0;
}

const char *argparser_error(struct argparser *ap)
{
        return ap->error;
}

uint32_t argparser_count(struct argparser *ap)
{
        return ap->nval;
}

const char *argparser_val(struct argparser *ap, uint32_t index)
{
        if (index >= MAX_VAL)
                return NULL;

        return ap->vals[index];
}

const char *argparser_help(struct argparser *ap)
{
        size_t n = 0;
        struct option *opt;

        if (ap->name)
                n += snprintf(ap->help, sizeof(ap->help), "Usage: %s [options]\n", ap->name);

        n += snprintf(ap->help + n, sizeof(ap->help) - n, "Options:\n");

        for (int i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];

                if (opt->shortopt) {
                        n += snprintf(ap->help + n, sizeof(ap->help) - n, "  -%s", opt->shortopt);
                } else {
                        n += snprintf(ap->help + n, sizeof(ap->help) - n, "  ");
                }

                if (opt->longopt) {
                        if (opt->shortopt) {
                                n += snprintf(ap->help + n, sizeof(ap->help) - n, ", --%s", opt->longopt);
                        } else {
                                n += snprintf(ap->help + n, sizeof(ap->help) - n, "--%s", opt->longopt);
                        }
                }

                if (opt->tips)
                        n += snprintf(ap->help + n, sizeof(ap->help) - n, "\n    %s\n", opt->tips);

                n += snprintf(ap->help + n, sizeof(ap->help) - n, "\n");
        }

        return ap->help;
}