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
 *    Type                    | Space   | Equals  | Concatenated
 *    -----------------------|---------|----------|-------------
 *    Single-char (-O)       | -O 123  | -O=123   | -O123 (opt_concat)
 *    Multi-char str (-abc)  | -abc 123| -abc=123 | ✗
 *    Option group (-xyz)    | -xyz 123| ✗        | ✗
 *  - Supports multiple values via space-separation or repeated options.
 *  - Support a single-character short option to group with other single-character
 *    short option.
 *
 * Code example:
 *
 * int main(int argc, char *argv[])
 * {
 *      argparser_t ap;
 *      option_t    std;
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

typedef struct _private_option // NOLINT(*-reserved-identifier)
{
        option_t option;

        /* built-in */
        uint32_t _capacity;
        option_t** _refs;
        PFN_argparser_callback _cb;
} _private_option_t; // NOLINT(*-reserved-identifier)

struct argparser
{
        const char *name;
        const char *version;

        /* options */
        _private_option_t **opts;
        uint32_t nopt;
        uint32_t optcap;

        /* position value */
        const char **vals;
        uint32_t nval;
        uint32_t valcap;

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
                _private_option_t **tmp;
                ap->optcap *= 2;
                tmp = realloc(ap->opts, ap->optcap * sizeof(_private_option_t *));
                if (!tmp)
                        return -ENOMEM;
                ap->opts = tmp;
        }

        return 0;
}

static int ensure_values_capacity(struct argparser *ap)
{
        if (ap->nval >= ap->valcap) {
                const char **tmp;
                ap->valcap *= 2;
                tmp = realloc(ap->vals, ap->valcap * sizeof(const char *));
                if (!tmp)
                        return -ENOMEM;
                ap->vals = tmp;
        }

        return 0;
}

static int add_option(struct argparser *ap, _private_option_t *privopt)
{
        int r;
        if ((r = ensure_option_capacity(ap)) != 0)
                return r;

        ap->opts[ap->nopt++] = privopt;

        return 0;
}

static int store_position_val(struct argparser *ap, const char *val)
{
        int r;
        if ((r = ensure_values_capacity(ap)) != 0)
                return r;

        ap->vals[ap->nval++] = val;

        return 0;
}

static int store_option_val(struct argparser *ap,
                            _private_option_t *privopt,
                            int is_long,
                            char *tok,
                            const char *val)
{
        if (privopt->option.count > privopt->option.max) {
                error(ap, "%s%s option value out of %d", OPT_PREFIX(is_long), tok, privopt->option.max);
                return -EOVERFLOW;
        }

        if (!privopt->option.vals) {
                privopt->_capacity = 16;
                privopt->option.count = 0;
                privopt->option.vals = calloc(privopt->_capacity, sizeof(char *));
                if (!privopt->option.vals) {
                        error(ap, strerror(errno));
                        return -ENOMEM;
                }
        }

        if (privopt->option.count >= privopt->_capacity) {
                const char **tmp_vals;
                privopt->_capacity *= 2;
                tmp_vals = realloc(privopt->option.vals, sizeof(char *) * privopt->_capacity);
                if (!tmp_vals) {
                        error(ap, strerror(errno));
                        return -ENOMEM;
                }

                privopt->option.vals = tmp_vals;
        }

        privopt->option.vals[privopt->option.count++] = val;

        if (privopt->option.count == 1)
                privopt->option.sval = privopt->option.vals[0];

        return 0;
}

/* Try to take a value for option, if the option not needs a value
 * or max is zero, that return 0 also return options consume value
 * count. */
static int try_take_val(struct argparser *ap,
                        _private_option_t *privopt,
                        int is_long,
                        char *tok,
                        char *eqval,
                        int *i,
                        char *argv[])
{
        if (privopt->_refs)
                *privopt->_refs = &privopt->option;

        if (privopt->option.max <= 0) {
                if (privopt->option.flags & opt_reqval) {
                        error(ap, "option %s%s flag need requires a value, but max capacity is zero",
                              OPT_PREFIX(is_long), tok);
                        return -EINVAL;
                }
                return 0;
        }

        /* equal sign value */
        if (eqval) {
                store_option_val(ap, privopt, is_long, tok, eqval);
                return (int) privopt->option.count;
        }

        while (privopt->option.count < privopt->option.max) {
                char *val = argv[*i + 1];

                if (!val || val[0] == '-') {
                        if ((privopt->option.flags & opt_reqval) && privopt->option.count == 0) {
                                error(ap, "option %s%s missing required argument", OPT_PREFIX(is_long), tok);
                                return -EINVAL;
                        }
                        break;
                }

                store_option_val(ap, privopt, is_long, tok, val); // <- count++
                *i += 1;
        }

        return (int) privopt->option.count;
}

static _private_option_t *lookup_short_char(struct argparser *ap, const char shortopt)
{
        _private_option_t *privopt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                privopt = ap->opts[i];
                if (privopt->option.shortopt) {
                        if (strlen(privopt->option.shortopt) == 1 && shortopt == privopt->option.shortopt[0])
                                return privopt;
                }
        }

        return NULL;
}

static _private_option_t *lookup_short_str(struct argparser *ap, const char *shortopt)
{
        _private_option_t *privopt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                privopt = ap->opts[i];
                if (privopt->option.shortopt && strcmp(shortopt, privopt->option.shortopt) == 0)
                        return privopt;
        }

        return NULL;
}

static _private_option_t *lookup_long(struct argparser *ap, const char *longopt)
{
        _private_option_t *privopt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                privopt = ap->opts[i];
                if (privopt->option.longopt && strcmp(longopt, privopt->option.longopt) == 0)
                        return privopt;
        }

        return NULL;
}

static int handle_short_concat(struct argparser *ap, char *tok, int *i, char *argv[]) // NOLINT(*-reserved-identifier)
{
        char *defval = NULL;
        _private_option_t *privopt;
        char tmp[2];
        size_t len;

        len = strlen(tok);

        tmp[0] = tok[0];
        tmp[1] = '\0';
        privopt = lookup_short_str(ap, tmp);
        if (privopt != NULL && (privopt->option.flags & opt_concat)) {
                if (len > 1)
                        defval = tok + 1;

                int r = try_take_val(ap, privopt, SHORT, tmp, defval, i, argv);
                if (r < 0)
                        return r;

                return 1;
        }

        return 0;
}

static int handle_short_assign(struct argparser *ap, char *tok, int *i, char *argv[]) // NOLINT(*-reserved-identifier)
{
        char *eqval = NULL;
        _private_option_t *privopt;

        tok = strdup(tok);
        if (!tok) {
                error(ap, strerror(errno));
                return -ENOMEM;
        }

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;
                *eq = '\0';
        }

        privopt = lookup_short_str(ap, tok);
        if (privopt != NULL) {
                int r = try_take_val(ap, privopt, SHORT, tok, eqval, i, argv);
                free(tok);
                return r < 0 ? r : 1;

        }

        if (eqval) {
                error(ap, "unknown option: -%s", tok);
                free(tok);
                return -EINVAL;
        }

        free(tok);
        return 0;
}

static int handle_short_group(struct argparser *ap, char *tok, int *i, char *argv[]) // NOLINT(*-reserved-identifier)
{
        int has_val = 0;
        char has_val_opt = 0;
        _private_option_t *privopt;
        char tmp[2];

        for (int k = 0; tok[k]; k++) {
                privopt = lookup_short_char(ap, tok[k]);
                if (!privopt) {
                        error(ap, "unknown option: -%c", tok[k]);
                        return -EINVAL;
                }

                if (privopt->option.flags & opt_concat) {
                        error(ap, "invalid option -%c cannot be in a group", tok[k]);
                        return -EINVAL;
                }

                if (privopt->option.flags & opt_nogroup) {
                        error(ap, "option -%c cannot be used as a group", tok[k]);
                        return -EINVAL;
                }

                if (has_val && privopt->option.max > 0) {
                        error(ap, "option -%c does not accept a value, cause option -%c already acceped", tok[k], has_val_opt);
                        return -EINVAL;
                }

                tmp[0] = tok[k];
                tmp[1] = '\0';

                int r = try_take_val(ap, privopt, SHORT, tmp, NULL, i, argv);
                if (r < 0)
                        return r;

                if (r > 0) {
                        has_val = 1;
                        has_val_opt = tok[k];
                }
        }

        return 0;
}

static int handle_short(struct argparser *ap, int *i, char *tok, char *argv[])
{
        int r;

        /* check opt_concat flag */
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
        int r;
        char *eqval = NULL;
        _private_option_t *privopt;

        tok = strdup(tok);
        if (!tok) {
                error(ap, strerror(errno));
                return -ENOMEM;
        }

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;
                *eq = '\0';
        }

        privopt = lookup_long(ap, tok);
        if (!privopt) {
                error(ap, "unknown option: --%s", tok);
                free(tok);
                return -EINVAL;
        }

        r = try_take_val(ap, privopt, LONG, tok, eqval, i, argv);
        free(tok);
        return r < 0 ? r : 0;
}

int argparser_acb_help(struct argparser *ap, option_t *opt)
{
        (void) opt;
        printf("%s", argparser_help(ap));
        exit(0);
}

int argparser_acb_version(struct argparser *ap, option_t *opt)
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
        ap->valcap = MIN_CAP;
        ap->vals = calloc(ap->valcap, sizeof(*ap->vals));
        if (!ap->vals) {
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
                        _private_option_t *privopt = ap->opts[i];
                        if (privopt->option.vals != NULL)
                                free(privopt->option.vals);

                        free(ap->opts[i]);
                }
                free(ap->opts);
                ap->opts = NULL;
        }

        if (ap->vals)
                free(ap->vals);

        free(ap);
}

int argparser_add0(struct argparser *ap,
                   option_t **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, 0, tips, cb, flags);
}

int argparser_add1(struct argparser *ap,
                   option_t **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, 1, tips, cb, flags);
}

int argparser_addn(struct argparser *ap,
                   option_t **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags)
{
        int r;
        _private_option_t *privopt;

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

        privopt = calloc(1, sizeof(*privopt));
        if (!privopt)
                return -ENOMEM;

        privopt->option.shortopt = shortopt;
        privopt->option.longopt = longopt;
        privopt->option.max = max;
        privopt->option.tips = tips;
        privopt->option.flags = flags;
        privopt->_refs = result_slot;
        privopt->_cb = cb;

        r = add_option(ap, privopt);
        if (r != 0) {
                free(privopt);
                return r;
        }

        return r;
}

static int execacb(struct argparser *ap)
{
        int r;
        _private_option_t *privopt;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                privopt = ap->opts[i];
                if (*privopt->_refs != NULL && privopt->_cb != NULL) {
                        r = privopt->_cb(ap, &privopt->option);
                        if (r != 0)
                                return -1;
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
option_t *argparser_has(struct argparser *ap, const char *name)
{
        size_t size;
        _private_option_t *privopt;

        size = strlen(name);

        if (size > 1) {
                privopt = lookup_long(ap, name);
                if (privopt)
                        goto found;

                privopt = lookup_short_str(ap, name);
                if (privopt)
                        goto found;
        }

        privopt = lookup_short_char(ap, name[0]);
        if (privopt)
                goto found;

        return NULL;

found:
        return *privopt->_refs ? &privopt->option : NULL;
}

uint32_t argparser_count(struct argparser *ap)
{
        return ap->nval;
}

const char *argparser_val(struct argparser *ap, uint32_t index)
{
        if (index >= ap->nval)
                return NULL;

        return ap->vals[index];
}

const char *argparser_help(struct argparser *ap)
{
        size_t n = 0;
        size_t cap = sizeof(ap->help);
        option_t *opt;

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
                opt = &ap->opts[i]->option;

                if (opt->shortopt) {
                        APPEND("  -%s", opt->shortopt);
                } else {
                        APPEND("  ");
                }

                if (opt->longopt) {
                        if (opt->shortopt) {
                                APPEND(", --%s", opt->longopt);
                        } else {
                                APPEND("--%s", opt->longopt);
                        }
                }

                if (opt->flags & opt_reqval)
                        APPEND(" <value>");

                if (opt->tips)
                        APPEND("\n    %s\n", opt->tips);

                APPEND("\n");
        }

#undef APPEND
out:
        if (n >= cap)
                n = cap - 1;
        ap->help[n] = '\0';
        return ap->help;
}