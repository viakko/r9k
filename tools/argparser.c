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
#define LONG    1
#define SHORT   0

#define A_CMD   (1 << 1)
#define A_RUN   (1 << 2)

#define OPT_PREFIX(is_long) (is_long ? "--" : "-")

#define WARNING(fmt, ...) \
        fprintf(stderr, "WARNING: " fmt "", ##__VA_ARGS__)

typedef void (*fn_iterate_t)(size_t, void *);

struct option_hdr
{
        struct option pub;

        /* built-in */
        size_t _short_len;
        size_t _long_len;
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
        int stat_flags;

        /* options */
        struct option_hdr **opts;
        uint32_t nopt;
        uint32_t optcap;

        /* position value */
        const char **posvals;
        uint32_t nposval;
        uint32_t posvalcap;

        /* sub argparser */
        const char *cmd_desc;
        argparser_cmd_callback_t cmd_callback;
        struct argparser *cmd_next;
        struct argparser *cmd_tail;

        /* buff */
        char error[1024];
        char *help;
        size_t helplen;
        size_t helpcap;

        /* builtin */
        struct option *opt_h;
        struct option *opt_v;

        /* id */
        uint32_t _mulid;
};

static struct argparser *find_subcmd(struct argparser *ap, const char *name)
{
        if (!ap->cmd_next)
                return NULL;

        if (strcmp(ap->cmd_next->name, name) == 0)
                return ap->cmd_next;

        return find_subcmd(ap->cmd_next, name);
}

static struct option_hdr *find_hdr_slot(struct argparser *ap, struct option **slot)
{
        for (uint32_t i = 0; i < ap->nopt; i++) {
                struct option_hdr *op_hdr = ap->opts[i];
                if (op_hdr->_slot == slot)
                        return op_hdr;
        }
        return NULL;
}

static struct option_hdr *find_hdr_option(struct argparser *ap, const char *name)
{
        bool is_short_char;
        const char *lopt;
        const char *sopt;
        size_t len;

        if (!name || name[0] == '\0')
                return NULL;

        len = strlen(name);
        is_short_char = (name[1] == '\0');

        for (uint32_t i = 0; i < ap->nopt; i++) {
                struct option_hdr *op_hdr = ap->opts[i];

                lopt = op_hdr->pub.longopt;
                sopt = op_hdr->pub.shortopt;

                if (sopt) {
                        if (is_short_char && op_hdr->_short_len == 1 && sopt[0] == name[0])
                                return op_hdr;

                        if (op_hdr->_short_len == len && memcmp(sopt, name, len) == 0)
                                return op_hdr;
                }

                if (lopt) {
                        if (op_hdr->_long_len == len && memcmp(lopt, name, len) == 0)
                                return op_hdr;
                }
        }

        return NULL;
}

__attribute__((format(printf, 2, 3)))
static void _error(struct argparser *ap, const char *fmt, ...)
{
        va_list va;
        size_t n;

        n = snprintf(ap->error, sizeof(ap->error), "error: ");

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
                        return A_ERROR_NO_MEMORY;
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
                        return A_ERROR_NO_MEMORY;
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
        if ((r = ensure_values_capacity(ap)) != A_OK)
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
                _error(ap, "%s%s option value out of %d", OPT_PREFIX(is_long), tok, op_hdr->_maxval);
                return A_ERROR_TOO_MANY_VAL;
        }

        if (!op_hdr->pub.vals) {
                op_hdr->_valcap = 16;
                op_hdr->pub.nval = 0;
                op_hdr->pub.vals = calloc(op_hdr->_valcap, sizeof(char *));
                if (!op_hdr->pub.vals)
                        return A_ERROR_NO_MEMORY;
        }

        if (op_hdr->pub.nval >= op_hdr->_valcap) {
                const char **tmp_vals;
                op_hdr->_valcap *= 2;
                tmp_vals = realloc(op_hdr->pub.vals, sizeof(char *) * op_hdr->_valcap);
                if (!tmp_vals)
                        return A_ERROR_NO_MEMORY;

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
                       _error(ap, "%s%s conflicts with option %s%s",
                               OPT_PREFIX(is_long), tok,
                               ent->pub.shortopt ? "-" : "--",
                               ent->pub.shortopt ? ent->pub.shortopt : ent->pub.longopt);
                        return A_ERROR_CONFLICT;
                }

        }

        if (op_hdr->_maxval == 0) {
                if (op_hdr->_flags & O_REQUIRED) {
                        _error(ap, "option %s%s flag need requires a value, but max capacity is zero",
                               OPT_PREFIX(is_long), tok);
                        return A_ERROR_REQUIRED_VAL;
                }

                if (eqval) {
                        _error(ap, "option %s%s does not accept arguments",
                                   OPT_PREFIX(is_long), tok);
                        return A_ERROR_NO_ARG_ACCEPT;
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
                                _error(ap, "option %s%s missing required argument", OPT_PREFIX(is_long), tok);
                                return A_ERROR_REQUIRED_VAL;
                        }
                        break;
                }

                store_option_val(ap, op_hdr, is_long, tok, val); // <- count++
                *i += 1;
        }

        return (int) op_hdr->pub.nval;
}

static int handle_short_concat(struct argparser *ap, char *tok, int *i, char *argv[])
{
        char *defval = NULL;
        struct option_hdr *op_hdr;
        size_t len;
        char short_char_tmp[2];

        len = strlen(tok);
        short_char_tmp[1] = '\0';

        short_char_tmp[0] = tok[0];
        op_hdr = find_hdr_option(ap, short_char_tmp);
        if (op_hdr != NULL && (op_hdr->_flags & O_CONCAT)) {
                if (len > 1)
                        defval = tok + 1;

                int r = try_take_val(ap, op_hdr, SHORT, short_char_tmp, defval, i, argv);
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
        struct option_hdr *op_hdr = NULL;

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;

                size_t len = eq - tok;
                char name[len + 1];
                memcpy(name, tok, len);
                name[len] = '\0';

                op_hdr = find_hdr_option(ap, name);
                if (op_hdr != NULL) {
                        r = try_take_val(ap, op_hdr, SHORT, name, eqval, i, argv);
                        return r < 0 ? r : 1;
                }

                if (eqval) {
                        _error(ap, "unknown option: -%s", name);
                        return A_ERROR_UNKNOWN_OPT;
                }

                return r;
        }

        /* no equal signs */
        op_hdr = find_hdr_option(ap, tok);
        if (op_hdr != NULL) {
                r = try_take_val(ap, op_hdr, SHORT, tok, eqval, i, argv);
                return r < 0 ? r : 1;
        }

        return r;
}

static int handle_short_group(struct argparser *ap, char *tok, int *i, char *argv[])
{
        bool has_val = false;
        char has_val_opt = 0;
        struct option_hdr *op_hdr;
        char short_char_tmp[2];

        short_char_tmp[1] = '\0';

        for (int k = 0; tok[k]; k++) {
                short_char_tmp[0] = tok[k];
                op_hdr = find_hdr_option(ap, short_char_tmp);
                if (!op_hdr) {
                        _error(ap, "unknown option: -%c", tok[k]);
                        return A_ERROR_UNKNOWN_OPT;
                }

                if (op_hdr->_flags & O_CONCAT) {
                        _error(ap, "invalid option -%c cannot be in a group", tok[k]);
                        return A_ERROR_INVALID_GROUP;
                }

                if (op_hdr->_flags & O_NOGROUP) {
                        _error(ap, "option -%c cannot be used as a group", tok[k]);
                        return A_ERROR_INVALID_GROUP;
                }

                if (has_val && op_hdr->_maxval > 0) {
                        _error(ap, "option -%c does not accept a value, cause option -%c already acceped", tok[k], has_val_opt);
                        return A_ERROR_MULTI_VAL_OPTS;
                }

                short_char_tmp[0] = tok[k];

                int r = try_take_val(ap, op_hdr, SHORT, short_char_tmp, NULL, i, argv);
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
        int r;
        char *eqval = NULL;
        struct option_hdr *op_hdr = NULL;

        char *eq = strchr(tok, '=');
        if (eq) {
                eqval = eq + 1;

                size_t len = eq - tok;
                char name[len + 1];
                memcpy(name, tok, len);
                name[len] = '\0';

                op_hdr = find_hdr_option(ap, name);
                if (!op_hdr) {
                        _error(ap, "unknown option: --%s", name);
                        return A_ERROR_UNKNOWN_OPT;
                }

                r = try_take_val(ap, op_hdr, LONG, name, eqval, i, argv);
                return r < 0 ? r : 0;
        }

        /* no equal signs */
        op_hdr = find_hdr_option(ap, tok);
        if (!op_hdr) {
                _error(ap, "unknown option: --%s", tok);
                return A_ERROR_UNKNOWN_OPT;
        }

        r = try_take_val(ap, op_hdr, LONG, tok, eqval, i, argv);
        return r < 0 ? r : 0;
}

static void check_warn_exists(struct argparser *ap, const char *longopt, const char *shortopt)
{
        /* check exists */
        if (longopt && find_hdr_option(ap, longopt)) {
                WARNING("long option --%s already exists\n", longopt);
                return;
        }

        if (shortopt && find_hdr_option(ap, shortopt)) {
                WARNING("short option -%s already exists\n", shortopt);
                return;
        }
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

struct argparser *argparser_new(const char *name, const char *version)
{
        struct argparser *ap;

        ap = calloc(1, sizeof(*ap));
        if (!ap)
                return NULL;

        ap->name = name;
        ap->version = version;

        /* options */
        ap->optcap = MIN_CAP;
        ap->opts = calloc(ap->optcap, sizeof(struct option_hdr *));
        if (!ap->opts) {
                argparser_free(ap);
                return NULL;
        }

        /* values */
        ap->posvalcap = MIN_CAP;
        ap->posvals = calloc(ap->posvalcap, sizeof(*ap->posvals));
        if (!ap->posvals) {
                argparser_free(ap);
                return NULL;
        }

        ap->_mulid = 1;

        return ap;
}

struct argparser *argparser_create(const char *name, const char *version)
{
        struct argparser *ap;

        ap = argparser_new(name, version);
        if (!ap)
                return NULL;

        argparser_add0(ap, &ap->opt_h, "h", "help", "show this help message.", A_CALLBACK_HELP, 0);
        argparser_add0(ap, &ap->opt_v, "version", NULL, "show current version.", A_CALLBACK_VERSION, 0);

        return ap;
}

int argparser_cmd_register(struct argparser *parent,
                           const char *name,
                           const char *desc,
                           argparser_register_t reg,
                           argparser_cmd_callback_t cb)
{
        if (!parent)
                return A_ERROR_NULL_PARENT;

        struct argparser *ap;

        ap = argparser_create(name, parent->version);
        if (!ap)
                return A_ERROR_CREATE_FAIL;

        ap->stat_flags |= A_CMD;
        ap->cmd_desc = desc;
        ap->cmd_callback = cb;

        if (!parent->cmd_next) {
                parent->cmd_next = ap;
        } else {
                parent->cmd_tail->cmd_next = ap;
        }

        parent->cmd_tail = ap;

        if (reg)
                return reg(ap);

        return 0;
}

void argparser_free(struct argparser *ap)
{
        if (!ap)
                return;

        if (ap->opts) {
                for (uint32_t i = 0; i < ap->nopt; i++) {
                        struct option_hdr *op_hdr = ap->opts[i];
                        free(op_hdr->pub.vals);
                        free(op_hdr);
                }
                free(ap->opts);
        }

        if (ap->cmd_next)
                argparser_free(ap->cmd_next);

        if (ap->help)
                free(ap->help);

        free(ap->posvals);

        free(ap);
}

int argparser_add0(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *help,
                   argparser_callback_t cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, help, NULL, 0, cb, flags);
}

int argparser_add1(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *help,
                   const char *metavar,
                   argparser_callback_t cb,
                   uint32_t flags)
{
        return argparser_addn(ap, result_slot, shortopt, longopt, help, metavar, 1, cb, flags);
}

int argparser_addn(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *help,
                   const char *metavar,
                   int maxval,
                   argparser_callback_t cb,
                   uint32_t flags)
{
        int r;
        struct option_hdr *op_hdr;

        check_warn_exists(ap, longopt, shortopt);

        /* Initialize the user option pointer to NULL,
         * If the user provides this option in command line,
         * the pointer will be updated to point to the actual option object.
         * WARNING: This pointer becomes invalid after argparser_free(). */
        *result_slot = NULL;

        op_hdr = calloc(1, sizeof(*op_hdr));
        if (!op_hdr)
                return A_ERROR_NO_MEMORY;

        op_hdr->pub.shortopt = shortopt;
        op_hdr->pub.longopt = longopt;
        op_hdr->pub.help = help;
        op_hdr->pub.metavar = metavar;
        op_hdr->_maxval = maxval;
        op_hdr->_flags = flags;
        op_hdr->_slot = result_slot;
        op_hdr->_cb = cb;

        if (shortopt)
                op_hdr->_short_len = strlen(shortopt);

        if (longopt)
                op_hdr->_long_len = strlen(longopt);

        /* builtin_option_add does NOT free ap or ap->opts on success or failure */
        r = builtin_option_add(ap, op_hdr);
        if (r != 0) {
                free(op_hdr);
                return r;
        }

        return r;
}

static int ap_exec(struct argparser *ap)
{
        int r;
        struct option_hdr *op_hdr;

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];
                if (*op_hdr->_slot != NULL && op_hdr->_cb != NULL) {
                        r = op_hdr->_cb(ap, &op_hdr->pub);
                        if (r != A_OK)
                                return A_ERROR_CALLBACK_FAIL;
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
                op_hdr = find_hdr_slot(ap, slot);
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

static int _argparser_run0(struct argparser *ap, int argc, char *argv[])
{
        int r;
        int i = 1;
        char *tok = NULL;
        struct argparser *cmd = NULL;
        bool terminator = false;

        if (ap->stat_flags & A_RUN) {
                _error(ap, "already call argparser_run()");
                return A_ERROR_REPEATED_CALL;
        }

        /* mark already calls run */
        ap->stat_flags |= A_RUN;

        if (argv == NULL)
                return A_ERROR_NO_MEMORY;

        /* sub command argv copy */
        int cmd_argc = 0;
        char *cmd_argv[argc];

        if (argc > 1 && (cmd = find_subcmd(ap, argv[1])) != NULL) {
                i = 2; /* skip sub command */
                cmd_argv[cmd_argc++] = argv[1];
        }

        for (; i < argc; i++) {
                tok = argv[i];

                if (tok[0] == '-' && tok[1] == '-' && tok[2] == '\0') {
                        terminator = true;
                        continue;
                }

                if (terminator || tok[0] != '-') {
                        store_position_val(cmd ? cmd : ap, tok);
                        continue;
                }

                if (tok[1] == '-') {
                        tok += 2;

                        if (cmd && find_hdr_option(cmd, tok)) {
                                cmd_argv[cmd_argc++] = argv[i];
                                continue;
                        }

                        r = handle_long(ap, &i, tok, argv);
                        if (r != 0)
                                return r;

                        continue;
                }

                tok++; /* skip '-' */

                if (cmd && find_hdr_option(cmd, tok)) {
                        cmd_argv[cmd_argc++] = argv[i];
                        continue;
                }

                r = handle_short(ap, &i, tok, argv);
                if (r != 0)
                        return r;
        }

        /* if include cmd parsing for sub command. */
        if (cmd) {
                if ((r = _argparser_run0(cmd, cmd_argc, cmd_argv)) != 0) {
                        memcpy(ap->error, cmd->error, sizeof(ap->error));
                        return r;
                }
                cmd->cmd_callback(cmd);
        }

        r = ap_exec(ap);
        if (r != 0)
                return r;

        return 0;
}

int argparser_run(struct argparser *ap, int argc, char *argv[])
{
        if (!ap)
                return A_ERROR_NULL_ARGPARSER;

        if (ap->stat_flags & A_CMD) {
                _error(ap, "not allow sub argparser call argparser_run()");
                return A_ERROR_SUBCOMMAND_CALL;
        }

        return _argparser_run0(ap, argc, argv);
}

const char *argparser_error(struct argparser *ap)
{
        return ap->error;
}

/* Query an option with user input */
struct option *argparser_has(struct argparser *ap, const char *name)
{
        struct option_hdr *op_hdr;

        op_hdr = find_hdr_option(ap, name);
        if (!op_hdr)
                return NULL;

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

__attribute__((format(printf, 2, 3)))
static ssize_t _append_help(struct argparser *ap, const char *fmt, ...)
{
        ssize_t n;
        va_list va1, va2;

        va_start(va1, fmt);
        va_copy(va2, va1);
        n = vsnprintf(NULL, 0, fmt, va2);
        va_end(va2);

        if (n < 0) {
                va_end(va1);
                goto out;
        }

        /* ensure cap */
        if (ap->helplen + n >= ap->helpcap) {
                char *tmp;
                size_t newcap = (ap->helpcap * 2) + n + 1;
                tmp = realloc(ap->help, newcap);
                if (!tmp)
                        return A_ERROR_NO_MEMORY;
                ap->help = tmp;
                ap->helpcap = newcap; /* make sure the end of '\0' */
        }

        n = vsnprintf(ap->help + ap->helplen, ap->helpcap - ap->helplen, fmt, va1);
        va_end(va1);

        if (n >= 0) {
                ap->helplen += n;
                ap->help[ap->helplen] = '\0';
        }

out:
        return n;
}

const char *argparser_help(struct argparser *ap)
{
        struct argparser *next = NULL;
        struct option_hdr *op_hdr = NULL;

        if (ap->help)
                return ap->help;

        _append_help(ap, "Usage: \n");

        if (!(ap->stat_flags & A_CMD) && ap->cmd_next) {
                _append_help(ap, "  %s <commands> [options] [args]\n\n", ap->name);
                _append_help(ap, "Commands:\n");

                next = ap->cmd_next;
                while (next) {
                        _append_help(ap, "  %-18s %s\n", next->name, next->cmd_desc);
                        next = next->cmd_next;
                }

                _append_help(ap, "\nGlobal options:\n");
        } else {
                _append_help(ap, "  %s [options] [args]\n\n", ap->name);
                _append_help(ap, "Options:\n");
        }

        for (uint32_t i = 0; i < ap->nopt; i++) {
                op_hdr = ap->opts[i];
                char opt_buf[128] = "";
                int pos = 0;

                if (op_hdr->pub.shortopt) {
                        if (op_hdr->pub.longopt) {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "-%s, ", op_hdr->pub.shortopt);
                        } else {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "-%s", op_hdr->pub.shortopt);
                        }
                }

                if (op_hdr->pub.longopt)
                        pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "--%s", op_hdr->pub.longopt);

                if (op_hdr->_maxval > 0) {
                        if (op_hdr->_flags & O_REQUIRED) {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, " <%s>",
                                                op_hdr->pub.metavar ? op_hdr->pub.metavar : "value");
                        } else {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, " [%s]",
                                                op_hdr->pub.metavar ? op_hdr->pub.metavar : "value");
                        }

                        if (op_hdr->_maxval > 1)
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "...");
                }

                opt_buf[pos] = '\0';
                _append_help(ap, "  %-18s", opt_buf);

                if (op_hdr->pub.help)
                        _append_help(ap, " %s\n", op_hdr->pub.help);
        }

        _append_help(ap, "\n");

        if (ap->cmd_next) {
                _append_help(ap, "Run `%s <command> --help` for more information.", ap->name);
        } else {
                _append_help(ap, "Run `%s --help` for more information.", ap->name);
        }

        _append_help(ap, "\n");

        return ap->help;
}