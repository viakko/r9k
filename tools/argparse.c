/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include <r9k/argparse.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define INIT_VEC_CAP 8
#define INIT_VAL_CAP 1
#define INIT_ERR_CAP 512

#define A_STAT_CMD  (1 << 1)
#define A_STAT_RUN  (1 << 2)

#define OPT_PREFIX(is_long) (is_long ? "--" : "-")

#define WARNING(fmt, ...) \
        fprintf(stderr, "WARNING: " fmt "", ##__VA_ARGS__)

struct ptrvec
{
        void **items;
        size_t count;
        size_t cap;
};

static int ptrvec_init(struct ptrvec *p_vec)
{
        p_vec->items = calloc(INIT_VEC_CAP, sizeof(void *));
        if (!p_vec->items)
                return A_ERROR_NO_MEMORY;

        p_vec->count = 0;
        p_vec->cap   = INIT_VEC_CAP;

        return A_OK;
}

static void ptrvec_free(struct ptrvec *vec)
{
        if (vec->items)
                free(vec->items);
}

static size_t ptrvec_count(struct ptrvec *vec)
{
        return vec->count;
}

static void *ptrvec_fetch(struct ptrvec *vec, size_t index)
{
        if (index >= vec->count)
                return NULL;

        return vec->items[index];
}

static int ptrvec_push_back(struct ptrvec *vec, void *items)
{
        if (vec->count >= vec->cap) {
                size_t newcap = vec->cap ? vec->cap * 2 : INIT_VEC_CAP;
                void **newdata = realloc(vec->items, newcap * sizeof(void *));
                if (!newdata)
                        return A_ERROR_NO_MEMORY;
                vec->items = newdata;
                vec->cap = newcap;
        }

        vec->items[vec->count++] = items;
        return 0;
}

struct option_hdr
{
        struct option view;

        /* built-in */
        size_t _short_len;
        size_t _long_len;
        uint32_t _valcap;
        struct option** _slot; /* if an input option using _slot writes back the pointer. */
        struct option *_self_ptr; /* if _slot is NULL, use _self_ptr */
        argparse_callback_t _cb;
        uint32_t _maxval;
        uint32_t _flags;
        uint32_t _mulid; /* mutual group id */
};

struct argparse
{
        const char *name;
        const char *version;
        int stat_flags;

        /* options */
        struct ptrvec opt_vec;

        /* position value */
        struct ptrvec posval_vec;

        /* sub argparse */
        const char *cmd_desc;
        argparse_cmd_callback_t cmd_callback;
        struct argparse *cmd_next;
        struct argparse *cmd_tail;

        /* buff */
        char error[INIT_ERR_CAP];
        char *help;
        size_t hlen;
        size_t hcap;

        /* builtin */
        struct option *opt_h;
        struct option *opt_v;

        /* id */
        uint32_t _mulid;
};

static struct argparse *find_subcmd(struct argparse *ap, const char *name)
{
        struct argparse *p = ap->cmd_next;

        while (p) {
                if (strcmp(p->name, name) == 0)
                        return p;
                p = p->cmd_next;
        }

        return NULL;
}

static struct option_hdr *find_hdr_slot(struct argparse *ap, struct option **slot)
{
        for (uint32_t i = 0; i < ptrvec_count(&ap->opt_vec); i++) {
                struct option_hdr *op_hdr = ptrvec_fetch(&ap->opt_vec, i);
                if (op_hdr->_slot == slot)
                        return op_hdr;
        }
        return NULL;
}

static struct option_hdr *find_hdr_option(struct argparse *ap, const char *name)
{
        bool is_short_char;
        const char *lopt;
        const char *sopt;
        size_t len;

        if (!name || name[0] == '\0')
                return NULL;

        len = strlen(name);
        is_short_char = (name[1] == '\0');

        for (uint32_t i = 0; i < ptrvec_count(&ap->opt_vec); i++) {
                struct option_hdr *op_hdr = ptrvec_fetch(&ap->opt_vec, i);

                lopt = op_hdr->view.longopt;
                sopt = op_hdr->view.shortopt;

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
static void error_rec(struct argparse *ap, const char *fmt, ...)
{
        va_list va;
        size_t n;

        n = snprintf(ap->error, sizeof(ap->error), "error: ");

        va_start(va, fmt);
        vsnprintf(ap->error + n, sizeof(ap->error) - n, fmt, va);
        va_end(va);
}

static uint32_t getmulid(struct argparse *ap)
{
        return ap->_mulid++;
}

static int store_position_val(struct argparse *ap, char *val)
{
        return ptrvec_push_back(&ap->posval_vec, val);
}

static int store_option_val(struct argparse *ap,
                            struct option_hdr *op_hdr,
                            int is_long,
                            char *tok,
                            const char *val)
{
        if (op_hdr->view.nval > op_hdr->_maxval) {
                error_rec(ap, "%s%s option value out of %d", OPT_PREFIX(is_long), tok, op_hdr->_maxval);
                return A_ERROR_TOO_MANY_VAL;
        }

        if (!op_hdr->view.vals) {
                op_hdr->_valcap = INIT_VAL_CAP;
                op_hdr->view.nval = 0;
                op_hdr->view.vals = calloc(op_hdr->_valcap, sizeof(char *));
                if (!op_hdr->view.vals) {
                        error_rec(ap, "out of memory");
                        return A_ERROR_NO_MEMORY;
                }
        }

        if (op_hdr->view.nval >= op_hdr->_valcap) {
                const char **tmp_vals;
                op_hdr->_valcap *= 2;
                tmp_vals = realloc(op_hdr->view.vals, sizeof(char *) * op_hdr->_valcap);
                if (!tmp_vals) {
                        error_rec(ap, "out of memory");
                        return A_ERROR_NO_MEMORY;
                }

                op_hdr->view.vals = tmp_vals;
        }

        op_hdr->view.vals[op_hdr->view.nval++] = val;

        if (op_hdr->view.nval == 1)
                op_hdr->view.sval = op_hdr->view.vals[0];

        return 0;
}

static struct option_hdr *is_mutual(struct argparse *ap, struct option_hdr *op_hdr)
{
        struct option_hdr *ent;

        if (op_hdr->_mulid == 0)
                return NULL;

        for (uint32_t i = 0; i < ptrvec_count(&ap->opt_vec); i++) {
                ent = ptrvec_fetch(&ap->opt_vec, i);
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
static int try_take_val(struct argparse *ap,
                        struct option_hdr *op_hdr,
                        int is_long,
                        char *tok,
                        char *eqval,
                        int *i,
                        char *argv[])
{
        if (op_hdr->_slot) {
                *op_hdr->_slot = &op_hdr->view;

                struct option_hdr *ent = is_mutual(ap, op_hdr);
                if (ent) {
                       error_rec(ap, "%s%s conflicts with option %s%s",
                               OPT_PREFIX(is_long), tok,
                               ent->view.shortopt ? "-" : "--",
                               ent->view.shortopt ? ent->view.shortopt : ent->view.longopt);
                        return A_ERROR_CONFLICT;
                }

        }

        if (op_hdr->_maxval == 0) {
                if (op_hdr->_flags & O_REQUIRED) {
                        error_rec(ap, "option %s%s flag need requires a value, but max capacity is zero",
                               OPT_PREFIX(is_long), tok);
                        return A_ERROR_REQUIRED_VAL;
                }

                if (eqval) {
                        error_rec(ap, "option %s%s does not accept arguments",
                                   OPT_PREFIX(is_long), tok);
                        return A_ERROR_NO_ARG_ACCEPT;
                }
                return 0;
        }

        /* equal sign value */
        if (eqval) {
                store_option_val(ap, op_hdr, is_long, tok, eqval);
                return (int) op_hdr->view.nval;
        }

        while (op_hdr->view.nval < op_hdr->_maxval) {
                char *val = argv[*i + 1];

                if (!val || val[0] == '-') {
                        if ((op_hdr->_flags & O_REQUIRED) && op_hdr->view.nval == 0) {
                                error_rec(ap, "option %s%s missing required argument", OPT_PREFIX(is_long), tok);
                                return A_ERROR_REQUIRED_VAL;
                        }
                        break;
                }

                store_option_val(ap, op_hdr, is_long, tok, val); // <- count++
                *i += 1;
        }

        return (int) op_hdr->view.nval;
}

static int handle_short_concat(struct argparse *ap, char *tok, int *i, char *argv[])
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

                int r = try_take_val(ap, op_hdr, false, short_char_tmp, defval, i, argv);
                if (r < 0)
                        return r;

                return 1;
        }

        return 0;
}

static int handle_short_assign(struct argparse *ap, char *tok, int *i, char *argv[])
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
                        r = try_take_val(ap, op_hdr, false, name, eqval, i, argv);
                        return r < 0 ? r : 1;
                }

                if (eqval) {
                        error_rec(ap, "unknown option: -%s", name);
                        return A_ERROR_UNKNOWN_OPT;
                }

                return r;
        }

        /* no equal signs */
        op_hdr = find_hdr_option(ap, tok);
        if (op_hdr != NULL) {
                r = try_take_val(ap, op_hdr, false, tok, eqval, i, argv);
                return r < 0 ? r : 1;
        }

        return r;
}

static int handle_short_group(struct argparse *ap, char *tok, int *i, char *argv[])
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
                        error_rec(ap, "unknown option: -%c", tok[k]);
                        return A_ERROR_UNKNOWN_OPT;
                }

                if (op_hdr->_flags & O_CONCAT) {
                        error_rec(ap, "invalid option -%c cannot be in a group", tok[k]);
                        return A_ERROR_INVALID_GROUP;
                }

                if (op_hdr->_flags & O_NOGROUP) {
                        error_rec(ap, "option -%c cannot be used as a group", tok[k]);
                        return A_ERROR_INVALID_GROUP;
                }

                if (has_val && op_hdr->_maxval > 0) {
                        error_rec(ap, "option -%c does not accept a value, cause option -%c already acceped", tok[k], has_val_opt);
                        return A_ERROR_MULTI_VAL_OPTS;
                }

                short_char_tmp[0] = tok[k];

                int r = try_take_val(ap, op_hdr, false, short_char_tmp, NULL, i, argv);
                if (r < 0)
                        return r;

                if (r > 0) {
                        has_val = true;
                        has_val_opt = tok[k];
                }
        }

        return 0;
}

static int handle_short(struct argparse *ap, int *i, char *tok, char *argv[])
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

static int handle_long(struct argparse *ap, int *i, char *tok, char *argv[])
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
                        error_rec(ap, "unknown option: --%s", name);
                        return A_ERROR_UNKNOWN_OPT;
                }

                r = try_take_val(ap, op_hdr, true, name, eqval, i, argv);
                return r < 0 ? r : 0;
        }

        /* no equal signs */
        op_hdr = find_hdr_option(ap, tok);
        if (!op_hdr) {
                error_rec(ap, "unknown option: --%s", tok);
                return A_ERROR_UNKNOWN_OPT;
        }

        r = try_take_val(ap, op_hdr, true, tok, eqval, i, argv);
        return r < 0 ? r : 0;
}

static void check_warn_exists(struct argparse *ap, const char *longopt, const char *shortopt)
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

int _argparse_callback_help(struct argparse *ap, struct option *opt)
{
        (void) opt;
        printf("%s", argparse_help(ap));
        exit(0);
}

int _argparse_callback_version(struct argparse *ap, struct option *opt)
{
        (void) opt;
        printf("%s %s\n", ap->name, ap->version);
        exit(0);
}

struct argparse *argparse_new(const char *name, const char *version)
{
        struct argparse *ap;

        ap = calloc(1, sizeof(*ap));
        if (!ap)
                return NULL;

        ap->name = name;
        ap->version = version;
        ap->_mulid = 1;

        /* options */
        if (ptrvec_init(&ap->opt_vec) != A_OK) {
                argparse_free(ap);
                return NULL;
        }

        /* values */
        if (ptrvec_init(&ap->posval_vec) != A_OK) {
                argparse_free(ap);
                return NULL;
        }

        return ap;
}

struct argparse *argparse_create(const char *name, const char *version)
{
        struct argparse *ap;

        ap = argparse_new(name, version);
        if (!ap)
                return NULL;

        argparse_add0(ap, &ap->opt_h, "h", "help", "show this help message.", A_CALLBACK_HELP, 0);
        argparse_add0(ap, &ap->opt_v, "version", NULL, "show current version.", A_CALLBACK_VERSION, 0);

        return ap;
}

int argparse_cmd(struct argparse *parent,
                 const char *name,
                 const char *desc,
                 argparse_register_t reg,
                 argparse_cmd_callback_t cb)
{
        if (!parent)
                return A_ERROR_NULL_PARENT;

        struct argparse *ap;

        ap = argparse_create(name, parent->version);
        if (!ap)
                return A_ERROR_CREATE_FAIL;

        ap->stat_flags |= A_STAT_CMD;
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

// NOLINTNEXTLINE(misc-no-recursion)
void argparse_free(struct argparse *ap)
{
        if (!ap)
                return;

        for (uint32_t i = 0; i < ptrvec_count(&ap->opt_vec); i++) {
                struct option_hdr *op_hdr = ptrvec_fetch(&ap->opt_vec, i);
                free(op_hdr->view.vals);
                *op_hdr->_slot = NULL;
                free(op_hdr);
        }
        ptrvec_free(&ap->opt_vec);

        if (ap->cmd_next)
                argparse_free(ap->cmd_next);

        if (ap->help)
                free(ap->help);

        ptrvec_free(&ap->posval_vec);

        free(ap);
}

int argparse_add0(struct argparse *ap,
                  struct option **result_slot,
                  const char *shortopt,
                  const char *longopt,
                  const char *help,
                  argparse_callback_t cb,
                  uint32_t flags)
{
        return argparse_addn(ap, result_slot, shortopt, longopt, help, NULL, 0, cb, flags);
}

int argparse_add1(struct argparse *ap,
                  struct option **result_slot,
                  const char *shortopt,
                  const char *longopt,
                  const char *help,
                  const char *metavar,
                  argparse_callback_t cb,
                  uint32_t flags)
{
        return argparse_addn(ap, result_slot, shortopt, longopt, help, metavar, 1, cb, flags);
}

int argparse_addn(struct argparse *ap,
                  struct option **result_slot,
                  const char *shortopt,
                  const char *longopt,
                  const char *help,
                  const char *metavar,
                  int maxval,
                  argparse_callback_t cb,
                  uint32_t flags)
{
        int r;
        struct option_hdr *op_hdr;

        if (ap->stat_flags & A_STAT_RUN) {
                error_rec(ap, "after call argparse_run()");
                return A_ERROR_AFTER_RUN;
        }

        check_warn_exists(ap, longopt, shortopt);

        op_hdr = calloc(1, sizeof(*op_hdr));
        if (!op_hdr) {
                error_rec(ap, "out of memory");
                return A_ERROR_NO_MEMORY;
        }

        /* Initialize the user option pointer to NULL,
         * If the user provides this option in command line,
         * the pointer will be updated to point to the actual option object.
         * WARNING: This pointer becomes invalid after argparse_free(). */
        if (!result_slot)
                result_slot = &op_hdr->_self_ptr;
        *result_slot = NULL;

        op_hdr->view.shortopt = shortopt;
        op_hdr->view.longopt = longopt;
        op_hdr->view.help = help;
        op_hdr->view.metavar = metavar;
        op_hdr->_maxval = maxval;
        op_hdr->_flags = flags;
        op_hdr->_slot = result_slot;
        op_hdr->_cb = cb;

        if (shortopt)
                op_hdr->_short_len = strlen(shortopt);

        if (longopt)
                op_hdr->_long_len = strlen(longopt);

        r = ptrvec_push_back(&ap->opt_vec, op_hdr);
        if (r != 0) {
                free(op_hdr);
                return r;
        }

        return r;
}

static int callback_exec(struct argparse *ap)
{
        int r;
        struct option_hdr *op_hdr;

        for (uint32_t i = 0; i < ptrvec_count(&ap->opt_vec); i++) {
                op_hdr = ptrvec_fetch(&ap->opt_vec, i);
                if (*op_hdr->_slot != NULL && op_hdr->_cb != NULL) {
                        r = op_hdr->_cb(ap, &op_hdr->view);
                        if (r != A_OK)
                                return A_ERROR_CALLBACK_FAIL;
                }
        }

        return 0;
}

void _argparse_mutual_exclude(struct argparse *ap, ...)
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
                                op_hdr->view.shortopt ? "-" : "--",
                                op_hdr->view.shortopt ? op_hdr->view.shortopt : op_hdr->view.longopt);

                op_hdr->_mulid = mulid;
        }
        va_end(va);
}

static int _argparse_run(struct argparse *ap, int argc, char *argv[])
{
        int r;
        int i = 1;
        char *tok = NULL;
        struct argparse *cmd = NULL;
        bool terminator = false;

        if (ap->stat_flags & A_STAT_RUN) {
                error_rec(ap, "already call argparse_run()");
                r = A_ERROR_AFTER_RUN;
                goto out;
        }

        /* mark already calls run */
        ap->stat_flags |= A_STAT_RUN;

        if (argv == NULL) {
                r = A_ERROR_NO_MEMORY;
                goto out;
        }

        /* sub command argv copy */
        struct ptrvec arg_vec;
        if ((r = ptrvec_init(&arg_vec)) != A_OK)
                goto out;

        if (argc > 1 && (cmd = find_subcmd(ap, argv[1])) != NULL) {
                i = 2; /* skip sub command */
                if ((r = ptrvec_push_back(&arg_vec, argv[1])) != A_OK)
                        goto out;
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
                                if ((r = ptrvec_push_back(&arg_vec, argv[i])) != A_OK)
                                        goto out;
                                continue;
                        }

                        r = handle_long(ap, &i, tok, argv);
                        if (r != 0)
                                goto out;

                        continue;
                }

                tok++; /* skip '-' */

                if (cmd && find_hdr_option(cmd, tok)) {
                        if ((r = ptrvec_push_back(&arg_vec, argv[i])) != A_OK)
                                goto out;
                        continue;
                }

                r = handle_short(ap, &i, tok, argv);
                if (r != 0)
                        goto out;
        }

        /* if include cmd parsing for sub command. */
        if (cmd) {
                if ((r = _argparse_run(cmd, (int) ptrvec_count(&arg_vec), (char **) arg_vec.items)) != 0) {
                        memcpy(ap->error, cmd->error, sizeof(ap->error));
                        goto out;
                }

                r = cmd->cmd_callback(cmd);
                if (r != A_OK) {
                        error_rec(ap, "%s: callback fail", cmd->name);
                        goto out;
                }
        }

        r = callback_exec(ap);

out:
        ptrvec_free(&arg_vec);

        if (r == A_ERROR_NO_MEMORY)
                error_rec(ap, "out of memory");

        return r;
}

int argparse_run(struct argparse *ap, int argc, char *argv[])
{
        if (!ap)
                return A_ERROR_NULL_ARGPARSER;

        if (ap->stat_flags & A_STAT_CMD) {
                error_rec(ap, "not allow sub argparse call argparse_run()");
                return A_ERROR_SUBCOMMAND_CALL;
        }

        return _argparse_run(ap, argc, argv);
}

const char *argparse_error(struct argparse *ap)
{
        return ap->error;
}

/* Query an option with user input */
struct option *argparse_has(struct argparse *ap, const char *name)
{
        struct option_hdr *op_hdr;

        op_hdr = find_hdr_option(ap, name);
        if (!op_hdr)
                return NULL;

        return *op_hdr->_slot ? &op_hdr->view : NULL;
}

uint32_t argparse_count(struct argparse *ap)
{
        return ptrvec_count(&ap->posval_vec);
}

const char *argparse_val(struct argparse *ap, uint32_t index)
{
        if (index >= ptrvec_count(&ap->posval_vec))
                return NULL;

        return ptrvec_fetch(&ap->posval_vec, index);
}

__attribute__((format(printf, 2, 3)))
static ssize_t _append_help(struct argparse *ap, const char *fmt, ...) // NOLINT(*-reserved-identifier)
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
        if (ap->hlen + n >= ap->hcap) {
                char *tmp;
                size_t newcap = (ap->hcap * 2) + n + 1;
                tmp = realloc(ap->help, newcap);
                if (!tmp) {
                        error_rec(ap, "out of memory");
                        return A_ERROR_NO_MEMORY;
                }
                ap->help = tmp;
                ap->hcap = newcap; /* make sure the end of '\0' */
        }

        n = vsnprintf(ap->help + ap->hlen, ap->hcap - ap->hlen, fmt, va1);
        va_end(va1);

        if (n >= 0) {
                ap->hlen += n;
                ap->help[ap->hlen] = '\0';
        }

out:
        return n;
}

const char *argparse_help(struct argparse *ap)
{
        struct argparse *next = NULL;
        struct option_hdr *op_hdr = NULL;

        if (ap->help)
                return ap->help;

        _append_help(ap, "Usage: \n");

        if (!(ap->stat_flags & A_STAT_CMD) && ap->cmd_next) {
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

        for (uint32_t i = 0; i < ptrvec_count(&ap->opt_vec); i++) {
                op_hdr = ptrvec_fetch(&ap->opt_vec, i);
                char opt_buf[128] = "";
                int pos = 0;

                if (op_hdr->view.shortopt) {
                        if (op_hdr->view.longopt) {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "-%s, ", op_hdr->view.shortopt);
                        } else {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "-%s", op_hdr->view.shortopt);
                        }
                }

                if (op_hdr->view.longopt)
                        pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "--%s", op_hdr->view.longopt);

                if (op_hdr->_maxval > 0) {
                        if (op_hdr->_flags & O_REQUIRED) {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, " <%s>",
                                                op_hdr->view.metavar ? op_hdr->view.metavar : "value");
                        } else {
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, " [%s]",
                                                op_hdr->view.metavar ? op_hdr->view.metavar : "value");
                        }

                        if (op_hdr->_maxval > 1)
                                pos += snprintf(opt_buf + pos, sizeof(opt_buf) - pos, "...");
                }

                opt_buf[pos] = '\0';
                _append_help(ap, "  %-18s", opt_buf);

                if (op_hdr->view.help)
                        _append_help(ap, " %s\n", op_hdr->view.help);
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