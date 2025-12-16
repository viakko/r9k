/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include <r9k/argparser.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MIN_CAP 8 /* default */
#define MAX_MSG 4096
#define LONG    1
#define SHORT   0

#define OPT_PREFIX(is_long) (is_long ? "--" : "-")

#define WARNING(fmt, ...) \
        fprintf(stderr, "WARNING: " fmt "", ##__VA_ARGS__)

struct option_hdr
{
        struct option pub;

        /* built-in */
        uint32_t _valcap;
        struct option** _slot; /* if an input option using _slot writes back the pointer. */
        argparser_callback_t _cb;
        uint32_t _maxval;
        uint32_t _flags;
        uint32_t _mulid; /* mutual group id */
};

struct argparser
{
        const char *name;
        const char *version;

        /* options */
        struct option_hdr **opts;
        uint32_t nopt;
        uint32_t optcap;

        /* position value */
        const char **posvals;
        uint32_t nposval;
        uint32_t posvalcap;

        /* buff */
        char error[MAX_MSG];
        char help[MAX_MSG];

        /* builtin */
        struct option *opt_h;
        struct option *opt_v;

        /* id */
        uint32_t _mulid;
};

static void ap_error(struct argparser *ap, const char *fmt, ...)
{
        va_list va;
        size_t n;

        n = snprintf(ap->error, sizeof(ap->error),
                     "%s: ", ap->name ? ap->name : "(null)");

        va_start(va, fmt);
        vsnprintf(ap->error + n, sizeof(ap->error) - n, fmt, va);
        va_end(va);
}

static uint32_t getmulid(struct argparser *ap)
{
        return ap->_mulid++;
}

static int ensure_option_capacity(struct argparser *ap)
{
        if (ap->nopt >= ap->optcap) {
                struct option_hdr **tmp;
                ap->optcap *= 2;
                tmp = realloc(ap->opts, ap->optcap * sizeof(struct option_hdr *));
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

static int builtin_option_add(struct argparser *ap, struct option_hdr *op_hdr)
{
        int r;
        if ((r = ensure_option_capacity(ap)) != 0)
                return r;

        ap->opts[ap->nopt++] = op_hdr;

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
                            struct option_hdr *op_hdr,
                            int is_long,
                            char *tok,
                            const char *val)
{
        if (op_hdr->pub.nval > op_hdr->_maxval) {
                ap_error(ap, "%s%s option value out of %d", OPT_PREFIX(is_long), tok, op_hdr->_maxval);
                return -EOVERFLOW;
        }

        if (!op_hdr->pub.vals) {
                op_hdr->_valcap = 16;
                op_hdr->pub.nval = 0;
                op_hdr->pub.vals = calloc(op_hdr->_valcap, sizeof(char *));
                if (!op_hdr->pub.vals) {
                        ap_error(ap, strerror(errno));
                        return -ENOMEM;
                }
        }

        if (op_hdr->pub.nval >= op_hdr->_valcap) {
                const char **tmp_vals;
                op_hdr->_valcap *= 2;
                tmp_vals = realloc(op_hdr->pub.vals, sizeof(char *) * op_hdr->_valcap);
                if (!tmp_vals) {
                        ap_error(ap, strerror(errno));
                        return -ENOMEM;
                }

                op_hdr->pub.vals = tmp_vals;
        }

        op_hdr->pub.vals[op_hdr->pub.nval++] = val;

        if (op_hdr->pub.nval == 1)
                op_hdr->pub.sval = op_hdr->pub.vals[0];

        return 0;
}

static struct option_hdr *is_mutual(struct argparser *ap, struct option_hdr *op_hdr)
{
        struct option_hdr *ent;

        if (op_hdr->_mulid == 0)
                return NULL;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                ent = ap->opts[i];
                if (ent == op_hdr)
                        continue;

                if (*ent->_slot != NULL && ent->_mulid == op_hdr->_mulid)
                        return ent;
        }

        return NULL;
}

/* Try to take a value for option, if the option not needs a value
 * or max is zero, that return 0 also return options consume value
 * count. */
static int try_take_val(struct argparser *ap,
                        struct option_hdr *op_hdr,
                        int is_long,
                        char *tok,
                        char *eqval,
                        int *i,
                        char *argv[])
{
        if (op_hdr->_slot) {
                *op_hdr->_slot = &op_hdr->pub;

                struct option_hdr *ent = is_mutual(ap, op_hdr);
                if (ent) {
                       ap_error(ap, "%s%s conflicts with option %s%s",
                               OPT_PREFIX(is_long), tok,
                               ent->pub.shortopt ? "-" : "--",
                               ent->pub.shortopt ? ent->pub.shortopt : ent->pub.longopt);
                        return -EINVAL;
                }

        }

        if (op_hdr->_maxval == 0) {
                if (op_hdr->_flags & O_REQUIRED) {
                        ap_error(ap, "option %s%s flag need requires a value, but max capacity is zero",
                              OPT_PREFIX(is_long), tok);
                        return -EINVAL;
                }
                return 0;
        }

        /* equal sign value */
        if (eqval) {
                store_option_val(ap, op_hdr, is_long, tok, eqval);
                return (int) op_hdr->pub.nval;
        }

        while (op_hdr->pub.nval < op_hdr->_maxval) {
                char *val = argv[*i + 1];

                if (!val || val[0] == '-') {
                        if ((op_hdr->_flags & O_REQUIRED) && op_hdr->pub.nval == 0) {
                                ap_error(ap, "option %s%s missing required argument", OPT_PREFIX(is_long), tok);
                                return -EINVAL;
                        }
                        break;
                }

                store_option_val(ap, op_hdr, is_long, tok, val); // <- count++
                *i += 1;
        }

        return (int) op_hdr->pub.nval;
}

static struct option_hdr *lookup_slot(struct argparser *ap, struct option **slot)
{
        for (uint32_t i = 0; i < ap->nopt; i++) {
                struct option_hdr *op_hdr = ap->opts[i];
                if (op_hdr->_slot == slot)
                        return op_hdr;
        }
        return NULL;
}

static struct option_hdr *lookup_short_char(struct argparser *ap, const char shortopt)
{
        struct option_hdr *op_hdr;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];
                if (op_hdr->pub.shortopt) {
                        if (strlen(op_hdr->pub.shortopt) == 1 && shortopt == op_hdr->pub.shortopt[0])
                                return op_hdr;
                }
        }

        return NULL;
}

static struct option_hdr *lookup_short_str(struct argparser *ap, const char *shortopt)
{
        struct option_hdr *op_hdr;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];
                if (op_hdr->pub.shortopt && strcmp(shortopt, op_hdr->pub.shortopt) == 0)
                        return op_hdr;
        }

        return NULL;
}

static struct option_hdr *lookup_long(struct argparser *ap, const char *longopt)
{
        struct option_hdr *op_hdr;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];
                if (op_hdr->pub.longopt && strcmp(longopt, op_hdr->pub.longopt) == 0)
                        return op_hdr;
        }

        return NULL;
}

static int handle_short_concat(struct argparser *ap, char *tok, int *i, char *argv[])
{
        char *defval = NULL;
        struct option_hdr *op_hdr;
        char tmp[2];
        size_t len;

        len = strlen(tok);

        tmp[0] = tok[0];
        tmp[1] = '\0';
        op_hdr = lookup_short_str(ap, tmp);
        if (op_hdr != NULL && (op_hdr->_flags & O_CONCAT)) {
                if (len > 1)
                        defval = tok + 1;

                int r = try_take_val(ap, op_hdr, SHORT, tmp, defval, i, argv);
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
        struct option_hdr *op_hdr;

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;
                *eq = '\0';
        }

        op_hdr = lookup_short_str(ap, tok);
        if (op_hdr != NULL) {
                r = try_take_val(ap, op_hdr, SHORT, tok, eqval, i, argv);
                return r < 0 ? r : 1;
        }

        if (eqval) {
                ap_error(ap, "unknown option: -%s", tok);
                return -EINVAL;
        }

        return r;
}

static int handle_short_group(struct argparser *ap, char *tok, int *i, char *argv[])
{
        bool has_val = false;
        char has_val_opt = 0;
        struct option_hdr *op_hdr;
        char tmp[2];

        for (int k = 0; tok[k]; k++) {
                op_hdr = lookup_short_char(ap, tok[k]);
                if (!op_hdr) {
                        ap_error(ap, "unknown option: -%c", tok[k]);
                        return -EINVAL;
                }

                if (op_hdr->_flags & O_CONCAT) {
                        ap_error(ap, "invalid option -%c cannot be in a group", tok[k]);
                        return -EINVAL;
                }

                if (op_hdr->_flags & O_NOGROUP) {
                        ap_error(ap, "option -%c cannot be used as a group", tok[k]);
                        return -EINVAL;
                }

                if (has_val && op_hdr->_maxval > 0) {
                        ap_error(ap, "option -%c does not accept a value, cause option -%c already acceped", tok[k], has_val_opt);
                        return -EINVAL;
                }

                tmp[0] = tok[k];
                tmp[1] = '\0';

                int r = try_take_val(ap, op_hdr, SHORT, tmp, NULL, i, argv);
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

        /* check O_CONCAT flag */
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
        struct option_hdr *op_hdr;

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;
                *eq = '\0';
        }

        op_hdr = lookup_long(ap, tok);
        if (!op_hdr) {
                ap_error(ap, "unknown option: --%s", tok);
                return -EINVAL;
        }

        r = try_take_val(ap, op_hdr, LONG, tok, eqval, i, argv);
        return r < 0 ? r : 0;
}

static int o_exists(struct argparser *ap, const char *longopt, const char *shortopt)
{
        /* check exists */
        if (longopt && lookup_long(ap, longopt)) {
                WARNING("long option --%s already exists\n", longopt);
                return -EINVAL;
        }

        if (shortopt && lookup_short_str(ap, shortopt)) {
                WARNING("short option -%s already exists\n", shortopt);
                return -EINVAL;
        }

        return 0;
}

int _argparser_builtin_callback_help(struct argparser *ap, struct option *op_hdr)
{
        (void) op_hdr;
        printf("%s", argparser_help(ap));
        exit(0);
}

int _argparser_builtin_callback_version(struct argparser *ap, struct option *op_hdr)
{
        (void) op_hdr;
        printf("%s %s\n", ap->name, ap->version);
        exit(0);
}

struct argparser *argparser_create_raw(const char *name, const char *version)
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
                ap_error(ap, strerror(errno));
                argparser_free(ap);
                return NULL;
        }

        /* values */
        ap->posvalcap = MIN_CAP;
        ap->posvals = calloc(ap->posvalcap, sizeof(*ap->posvals));
        if (!ap->posvals) {
                ap_error(ap, strerror(errno));
                argparser_free(ap);
                return NULL;
        }

        ap->_mulid = 1;

        return ap;
}

struct argparser *argparser_create(const char *name, const char *version)
{
        struct argparser *ap;

        ap = argparser_create_raw(name, version);
        if (!ap)
                return NULL;

        argparser_add0(ap, &ap->opt_h, "h", "help", "show this help message.", ACB_EXIT_HELP, 0);
        argparser_add0(ap, &ap->opt_v, "version", NULL, "show current version.", ACB_EXIT_VERSION, 0);

        return ap;
}

void argparser_free(struct argparser *ap)
{
        if (!ap)
                return;

        if (ap->opts) {
                for (uint32_t i = 0; i < ap->nopt; i++) {
                        struct option_hdr *op_hdr = ap->opts[i];
                        free(op_hdr->pub.vals);
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
                   argparser_callback_t cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, 0, tips, cb, flags);
}

int argparser_add1(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   argparser_callback_t cb,
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
                   argparser_callback_t cb,
                   uint32_t flags)
{
        int r;
        struct option_hdr *op_hdr;

        if ((r = o_exists(ap, longopt, shortopt)) != 0)
                return r;

        /* Initialize the user option pointer to NULL,
         * If the user provides this option in command line,
         * the pointer will be updated to point to the actual option object.
         * WARNING: This pointer becomes invalid after argparser_free(). */
        *result_slot = NULL;

        op_hdr = calloc(1, sizeof(*op_hdr));
        if (!op_hdr)
                return -ENOMEM;

        op_hdr->pub.shortopt = shortopt;
        op_hdr->pub.longopt = longopt;
        op_hdr->pub.tips = tips;
        op_hdr->_maxval = max;
        op_hdr->_flags = flags;
        op_hdr->_slot = result_slot;
        op_hdr->_cb = cb;

        /* builtin_option_add does NOT free ap or ap->opts on success or failure */
        r = builtin_option_add(ap, op_hdr);
        if (r != 0) {
                free(op_hdr);
                return r;
        }

        return r;
}

static int execacb(struct argparser *ap)
{
        int r;
        struct option_hdr *op_hdr;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];
                if (*op_hdr->_slot != NULL && op_hdr->_cb != NULL) {
                        r = op_hdr->_cb(ap, &op_hdr->pub);
                        if (r != 0)
                                return -EINVAL;
                }
        }

        return 0;
}

void _argparser_builtin_mutual_exclude(struct argparser *ap, ...)
{
        va_list va;
        struct option **slot;
        struct option_hdr *op_hdr;
        uint32_t mulid = getmulid(ap);

        va_start(va, ap);
        while ((slot = va_arg(va, struct option **)) != NULL) {
                op_hdr = lookup_slot(ap, slot);
                if (!op_hdr)
                        continue;

                if (op_hdr->_mulid != 0)
                        WARNING("option %s%s already in other mutual exclude group!\n",
                                op_hdr->pub.shortopt ? "-" : "--",
                                op_hdr->pub.shortopt ? op_hdr->pub.shortopt : op_hdr->pub.longopt);

                op_hdr->_mulid = mulid;
        }
        va_end(va);
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
        struct option_hdr *op_hdr;

        size = strlen(name);

        if (size > 1) {
                op_hdr = lookup_long(ap, name);
                if (op_hdr)
                        goto found;

                op_hdr = lookup_short_str(ap, name);
                if (op_hdr)
                        goto found;
        }

        op_hdr = lookup_short_char(ap, name[0]);
        if (op_hdr)
                goto found;

        return NULL;

found:
        return *op_hdr->_slot ? &op_hdr->pub : NULL;
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
        struct option_hdr *op_hdr;

#define APPEND(fmt, ...)                                        \
        do {                                                    \
                int __r;                                        \
                __r = snprintf(ap->help + n, cap - n, (fmt),    \
                               ##__VA_ARGS__);                  \
                if (__r < 0) {                                  \
                        ap_error(ap, strerror(errno));          \
                        return NULL;                            \
                }                                               \
                if ((size_t) __r >= cap - n)                    \
                        goto out;                               \
                n += __r;                                       \
        } while (0)

        if (ap->name)
                APPEND("Usage: %s [options]\n", ap->name);

        APPEND("Options:\n");

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];

                if (op_hdr->pub.shortopt) {
                        APPEND("  -%s", op_hdr->pub.shortopt);
                } else {
                        APPEND("  ");
                }

                if (op_hdr->pub.longopt) {
                        if (op_hdr->pub.shortopt) {
                                APPEND(", --%s", op_hdr->pub.longopt);
                        } else {
                                APPEND("--%s", op_hdr->pub.longopt);
                        }
                }

                if (op_hdr->_flags & O_REQUIRED)
                        APPEND(" <value>");

                if (op_hdr->pub.tips)
                        APPEND("\n    %s\n", op_hdr->pub.tips);

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