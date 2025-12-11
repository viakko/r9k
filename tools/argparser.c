/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * argparse - Lightweight command-line argument parsing library
 *
 * Provides multi-sytle command parsing with support for both
 * short and long options. Short option support string type,
 * and single-character options can be grouped.
 *
 * The rules:
 *  - If option like "-abc" not found in short option of definitions, it will be
 *    split into single-character options for short option matching.
 *  - Short option value syntax support:
 *    -----------------------------------------------------------------
 *    Type                   | Space   | Equals   | Concatenated
 *    -----------------------|---------|----------|--------------------
 *    Single-char (-O)       | -O 123  | -O=123   | -O123 (OPT_CONCAT)
 *    Multi-char str (-abc)  | -abc 123| -abc=123 | ✗
 *    Option group (-xyz)    | -xyz 123| ✗        | ✗
 *    -----------------------------------------------------------------
 *  - Supports multiple values via space-separation or repeated options.
 *  - Support a single-character short option to group with other single-character
 *    short option.
 *
 * Code example:
 *
 * int main(int argc, char *argv[])
 * {
 *      struct argparser ap;
 *      struct option    std;
 *
 *      ap = argparser_create("gcc", "1.0");
 *      if (!ap) {
 *              exit(0);
 *      }
 *
 *      if (argparser_run(ap, argc, argv) != 0)
 *              die(argparser_error(ap));
 *
 *      if (std)
 *              printf("std value: %s\n", std->sval);
 *
 *      return 0;
 * }
 *
 * NOTE: sval is a first value from vals[0], vals is values array.
 */
#include <r9k/argparser.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MIN_CAP 8 /* default */
#define MAX_MSG 4096
#define LONG    1
#define SHORT   0

#define OPT_PREFIX(is_long) (is_long ? "--" : "-")

typedef struct
{
        struct option pub;

        /* built-in */
        uint32_t _valcap;
        struct option** _refs; /* if an input option using _refs writes back the pointer. */
        PFN_argparser_callback _cb;
        uint32_t _flags;
} internal_option_t;

struct argparser
{
        const char *name;
        const char *version;

        /* options */
        internal_option_t **opts;
        uint32_t nopt;
        uint32_t optcap;

        /* position value */
        const char **posvals;
        uint32_t nposval;
        uint32_t posvalcap;

        /* buff */
        char error[MAX_MSG];
        char help[MAX_MSG];
};

static void error(struct argparser *ap, const char *fmt, ...)
{
        va_list va;
        va_start(va, fmt);
        vsnprintf(ap->error, sizeof(ap->error), fmt, va);
        va_end(va);
}

static int ensure_option_capacity(struct argparser *ap)
{
        if (ap->nopt >= ap->optcap) {
                internal_option_t **tmp;
                ap->optcap *= 2;
                tmp = realloc(ap->opts, ap->optcap * sizeof(internal_option_t *));
                if (!tmp)
                        return -ENOMEM;
                ap->opts = tmp;
        }

        return 0;
}

static int ensure_values_capacity(struct argparser *ap)
{
        if (ap->nposval >= ap->posvalcap) {
                const char **tmp;
                ap->posvalcap *= 2;
                tmp = realloc(ap->posvals, ap->posvalcap * sizeof(const char *));
                if (!tmp)
                        return -ENOMEM;
                ap->posvals = tmp;
        }

        return 0;
}

static int internal_option_add(struct argparser *ap, internal_option_t *opt)
{
        int r;
        if ((r = ensure_option_capacity(ap)) != 0)
                return r;

        ap->opts[ap->nopt++] = opt;

        return 0;
}

static int store_position_val(struct argparser *ap, const char *val)
{
        int r;
        if ((r = ensure_values_capacity(ap)) != 0)
                return r;

        ap->posvals[ap->nposval++] = val;

        return 0;
}

static int store_option_val(struct argparser *ap,
                            internal_option_t *opt, 
                            int is_long,
                            char *tok,
                            const char *val)
{
        if (opt->pub.nval > opt->pub.max) {
                error(ap, "%s%s option value out of %d", OPT_PREFIX(is_long), tok, opt->pub.max);
                return -EOVERFLOW;
        }

        if (!opt->pub.vals) {
                opt->_valcap = 16;
                opt->pub.nval = 0;
                opt->pub.vals = calloc(opt->_valcap, sizeof(char *));
                if (!opt->pub.vals) {
                        error(ap, strerror(errno));
                        return -ENOMEM;
                }
        }

        if (opt->pub.nval >= opt->_valcap) {
                const char **tmp_vals;
                opt->_valcap *= 2;
                tmp_vals = realloc(opt->pub.vals, sizeof(char *) * opt->_valcap);
                if (!tmp_vals) {
                        error(ap, strerror(errno));
                        return -ENOMEM;
                }

                opt->pub.vals = tmp_vals;
        }

        opt->pub.vals[opt->pub.nval++] = val;

        if (opt->pub.nval == 1)
                opt->pub.sval = opt->pub.vals[0];

        return 0;
}

/* Try to take a value for option, if the option not needs a value
 * or max is zero, that return 0 also return options consume value
 * count. */
static int try_take_val(struct argparser *ap,
                        internal_option_t *opt,
                        int is_long,
                        char *tok,
                        char *eqval,
                        int *i,
                        char *argv[])
{
        if (opt->_refs)
                *opt->_refs = &opt->pub;

        if (opt->pub.max <= 0) {
                if (opt->_flags & OPT_REQUIRED) {
                        error(ap, "option %s%s flag need requires a value, but max capacity is zero",
                              OPT_PREFIX(is_long), tok);
                        return -EINVAL;
                }
                return 0;
        }

        /* equal sign value */
        if (eqval) {
                store_option_val(ap, opt, is_long, tok, eqval);
                return (int) opt->pub.nval;
        }

        while (opt->pub.nval < opt->pub.max) {
                char *val = argv[*i + 1];

                if (!val || val[0] == '-') {
                        if ((opt->_flags & OPT_REQUIRED) && opt->pub.nval == 0) {
                                error(ap, "option %s%s missing required argument", OPT_PREFIX(is_long), tok);
                                return -EINVAL;
                        }
                        break;
                }

                store_option_val(ap, opt, is_long, tok, val); // <- count++
                *i += 1;
        }

        return (int) opt->pub.nval;
}

static internal_option_t *lookup_short_char(struct argparser *ap, const char shortopt)
{
        internal_option_t *opt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (opt->pub.shortopt) {
                        if (strlen(opt->pub.shortopt) == 1 && shortopt == opt->pub.shortopt[0])
                                return opt;
                }
        }

        return NULL;
}

static internal_option_t *lookup_short_str(struct argparser *ap, const char *shortopt)
{
        internal_option_t *opt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (opt->pub.shortopt && strcmp(shortopt, opt->pub.shortopt) == 0)
                        return opt;
        }

        return NULL;
}

static internal_option_t *lookup_long(struct argparser *ap, const char *longopt)
{
        internal_option_t *opt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (opt->pub.longopt && strcmp(longopt, opt->pub.longopt) == 0)
                        return opt;
        }

        return NULL;
}

static int handle_short_concat(struct argparser *ap, char *tok, int *i, char *argv[])
{
        char *defval = NULL;
        internal_option_t *opt;
        char tmp[2];
        size_t len;

        len = strlen(tok);

        tmp[0] = tok[0];
        tmp[1] = '\0';
        opt = lookup_short_str(ap, tmp);
        if (opt != NULL && (opt->_flags & OPT_CONCAT)) {
                if (len > 1)
                        defval = tok + 1;

                int r = try_take_val(ap, opt, SHORT, tmp, defval, i, argv);
                if (r < 0)
                        return r;

                return 1;
        }

        return 0;
}

static int handle_short_assign(struct argparser *ap, char *tok, int *i, char *argv[])
{
        int r = 0;
        char *eqval = NULL;
        internal_option_t *opt;

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;
                *eq = '\0';
        }

        opt = lookup_short_str(ap, tok);
        if (opt != NULL) {
                r = try_take_val(ap, opt, SHORT, tok, eqval, i, argv);
                return r < 0 ? r : 1;
        }

        if (eqval) {
                error(ap, "unknown option: -%s", tok);
                return -EINVAL;
        }

        return r;
}

static int handle_short_group(struct argparser *ap, char *tok, int *i, char *argv[])
{
        bool has_val = false;
        char has_val_opt = 0;
        internal_option_t *opt;
        char tmp[2];

        for (int k = 0; tok[k]; k++) {
                opt = lookup_short_char(ap, tok[k]);
                if (!opt) {
                        error(ap, "unknown option: -%c", tok[k]);
                        return -EINVAL;
                }

                if (opt->_flags & OPT_CONCAT) {
                        error(ap, "invalid option -%c cannot be in a group", tok[k]);
                        return -EINVAL;
                }

                if (opt->_flags & OPT_NOGRP) {
                        error(ap, "option -%c cannot be used as a group", tok[k]);
                        return -EINVAL;
                }

                if (has_val && opt->pub.max > 0) {
                        error(ap, "option -%c does not accept a value, cause option -%c already acceped", tok[k], has_val_opt);
                        return -EINVAL;
                }

                tmp[0] = tok[k];
                tmp[1] = '\0';

                int r = try_take_val(ap, opt, SHORT, tmp, NULL, i, argv);
                if (r < 0)
                        return r;

                if (r > 0) {
                        has_val = true;
                        has_val_opt = tok[k];
                }
        }

        return 0;
}

static int handle_short(struct argparser *ap, int *i, char *tok, char *argv[])
{
        int r;

        /* check OPT_CONCAT flag */
        r = handle_short_concat(ap, tok, i, argv);
        if (r > 0)
                return 0;

        if (r < 0)
                return r;

        /* check equal sign */
        r = handle_short_assign(ap, tok, i, argv);
        if (r > 0)
                return 0;

        if (r < 0)
                return r;

        return handle_short_group(ap, tok, i, argv);
}

static int handle_long(struct argparser *ap, int *i, char *tok, char *argv[])
{
        int r = 0;
        char *eqval = NULL;
        internal_option_t *opt;

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;
                *eq = '\0';
        }

        opt = lookup_long(ap, tok);
        if (!opt) {
                error(ap, "unknown option: --%s", tok);
                return -EINVAL;
        }

        r = try_take_val(ap, opt, LONG, tok, eqval, i, argv);
        return r < 0 ? r : 0;
}

int argparser_acb_help(struct argparser *ap, struct option *opt)
{
        (void) opt;
        printf("%s", argparser_help(ap));
        exit(0);
}

int argparser_acb_version(struct argparser *ap, struct option *opt)
{
        (void) opt;
        printf("%s %s\n", ap->name, ap->version);
        exit(0);
}

struct argparser *argparser_create(const char *name, const char *version)
{
        struct argparser *ap;

        ap = calloc(1, sizeof(*ap));
        if (!ap)
                return NULL;

        ap->name = name;
        ap->version = version;

        /* options */
        ap->optcap = MIN_CAP;
        ap->opts = calloc(ap->optcap, sizeof(*ap->opts));
        if (!ap->opts) {
                error(ap, strerror(errno));
                argparser_free(ap);
                return NULL;
        }

        /* values */
        ap->posvalcap = MIN_CAP;
        ap->posvals = calloc(ap->posvalcap, sizeof(*ap->posvals));
        if (!ap->posvals) {
                error(ap, strerror(errno));
                argparser_free(ap);
                return NULL;
        }

        return ap;
}

void argparser_free(struct argparser *ap)
{
        if (!ap)
                return;

        if (ap->opts) {
                for (uint32_t i = 0; i < ap->nopt; i++) {
                        internal_option_t *opt = ap->opts[i];
                        free(opt->pub.vals);
                        free(ap->opts[i]);
                }
                free(ap->opts);
        }

        free(ap->posvals);

        free(ap);
}

int argparser_add0(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, 0, tips, cb, flags);
}

int argparser_add1(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, 1, tips, cb, flags);
}

int argparser_addn(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags)
{
        int r;
        internal_option_t *opt;

        /* check exists */
        if (longopt && lookup_long(ap, longopt)) {
                error(ap, "long option --%s already exists", longopt);
                return -EINVAL;
        }

        if (shortopt && lookup_short_str(ap, shortopt)) {
                error(ap, "short option -%s already exists", shortopt);
                return -EINVAL;
        }

        /* Initialize the user option pointer to NULL,
         * If the user provides this option in command line,
         * the pointer will be updated to point to the actual option object.
         * WARNING: This pointer becomes invalid after argparser_free(). */
        *result_slot = NULL;

        opt = calloc(1, sizeof(*opt));
        if (!opt)
                return -ENOMEM;

        opt->pub.shortopt = shortopt;
        opt->pub.longopt = longopt;
        opt->pub.max = max;
        opt->pub.tips = tips;
        opt->_flags = flags;
        opt->_refs = result_slot;
        opt->_cb = cb;

        r = internal_option_add(ap, opt);
        if (r != 0) {
                free(opt);
                return r;
        }

        return r;
}

static int execacb(struct argparser *ap)
{
        int r;
        internal_option_t *opt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];
                if (*opt->_refs != NULL && opt->_cb != NULL) {
                        r = opt->_cb(ap, &opt->pub);
                        if (r != 0)
                                return -EINVAL;
                }
        }

        return 0;
}

int argparser_run(struct argparser *ap, int argc, char *argv[])
{
        int r;
        char *tok;
        bool terminator = false;

        if (!ap)
                return -EFAULT;

        for (int i = 1; i < argc; i++) {
                tok = argv[i];

                if (tok[0] == '-' && tok[1] == '-' && tok[2] == '\0') {
                        terminator = true;
                        continue;
                }

                if (terminator || tok[0] != '-') {
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

        r = execacb(ap);
        if (r != 0)
                return r;

        return 0;
}

const char *argparser_error(struct argparser *ap)
{
        return ap->error;
}

/* Query an option with user input */
struct option *argparser_has(struct argparser *ap, const char *name)
{
        size_t size;
        internal_option_t *opt;

        size = strlen(name);

        if (size > 1) {
                opt = lookup_long(ap, name);
                if (opt)
                        goto found;

                opt = lookup_short_str(ap, name);
                if (opt)
                        goto found;
        }

        opt = lookup_short_char(ap, name[0]);
        if (opt)
                goto found;

        return NULL;

found:
        return *opt->_refs ? &opt->pub : NULL;
}

uint32_t argparser_count(struct argparser *ap)
{
        return ap->nposval;
}

const char *argparser_val(struct argparser *ap, uint32_t index)
{
        if (index >= ap->nposval)
                return NULL;

        return ap->posvals[index];
}

const char *argparser_help(struct argparser *ap)
{
        size_t n = 0;
        size_t cap = sizeof(ap->help);
        internal_option_t *opt;

#define APPEND(fmt, ...)                                                \
        do {                                                            \
                int __r;                                                \
                __r = snprintf(ap->help + n, cap - n, (fmt),            \
                               ##__VA_ARGS__);                          \
                if (__r < 0) {                                          \
                        error(ap, strerror(errno));                     \
                        return NULL;                                    \
                }                                                       \
                if ((size_t) __r >= cap - n)                            \
                        goto out;                                       \
                n += __r;                                               \
        } while (0)

        if (ap->name)
                APPEND("Usage: %s [options]\n", ap->name);

        APPEND("Options:\n");

        for (uint32_t i = 0; i < ap->nopt; i++) {
                opt = ap->opts[i];

                if (opt->pub.shortopt) {
                        APPEND("  -%s", opt->pub.shortopt);
                } else {
                        APPEND("  ");
                }

                if (opt->pub.longopt) {
                        if (opt->pub.shortopt) {
                                APPEND(", --%s", opt->pub.longopt);
                        } else {
                                APPEND("--%s", opt->pub.longopt);
                        }
                }

                if (opt->_flags & OPT_REQUIRED)
                        APPEND(" <value>");

                if (opt->pub.tips)
                        APPEND("\n    %s\n", opt->pub.tips);

                if (i < ap->nopt - 1)
                        APPEND("\n");
        }

#undef APPEND
out:
        if (n >= cap)
                n = cap - 1;
        ap->help[n] = '\0';
        return ap->help;
}