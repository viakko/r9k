//
// Created by Ekko on 2025/11/19.
//
#include <argparse.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
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

static const struct optent *entfind(argparse_t *ap, const char *optname)
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

static size_t optcount(const struct option *opts)
{
        size_t n = 0;

        while (opts[n].long_name)
                n++;

        return n;
}

static ssize_t optfind(argparse_t *ap, const char *arg)
{
        ssize_t i;
        unsigned int is_short = 0;

        if (!arg || arg[0] != '-')
                return -1;

        if (arg[0] == '-' && arg[1] != '-') {
                arg += 1;
                is_short = 1;
        } else if (arg[0] == '-' && arg[1] == '-') {
                arg += 2;
        }

        if (is_short && strlen(arg) > 1) {
                seterror("short option bounding not supported: -%s", arg);
                return -2;
        }

        for (i = 0; i < ap->nopt; i++) {
                const struct option *op = &ap->opts[i];

                if ((is_short && op->short_name == arg[0])
                    || strcmp(op->long_name, arg) == 0)
                        return i;
        }

        return -1;
}

static int addval(argparse_t *ap, size_t optid, const char *val)
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

        if (ent != NULL && val != NULL && op->multi != opt_multi) {
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

                valdup = strdup(val);
                if (!valdup) {
                        seterror(strerror(errno));
                        return -1;
                }

                ent->vals = tmp_vals;
                ent->vals[ent->nval++] = valdup;
        }

        return 0;
}

argparse_t *argparse_parse(const struct option *opts, int argc, char **argv)
{
        ssize_t optid;
        argparse_t *ap;
        const struct option *op;

        ap = calloc(1, sizeof(*ap));
        ap->opts = opts;
        ap->nopt = optcount(opts);
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

                optid = optfind(ap, argv[i]);

                if (optid == -1)
                        continue;

                if (optid == -2) {
                        argparse_free(ap);
                        return NULL;
                }

                op = &ap->opts[optid];

                char *val = NULL;
                int next = i + 1;
                if (next < argc) {
                        if ((op->has_arg == required_argument) && argv[next][0] != '-') {
                                val = argv[i + 1];
                                ++i;
                        }
                }

                if (op->has_arg == required_argument && !val) {
                        seterror("option: <--%s> required argument", op->long_name);
                        argparse_free(ap);
                        return NULL;
                }

                if (addval(ap, optid, val) == -1) {
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
        return entfind(ap, name) ? 1 : 0;
}

const char **argparse_vals(argparse_t *ap, const char *name, size_t *nval)
{
        const struct optent *ent;

        ent = entfind(ap, name);
        if (!ent)
                return NULL;

        if (nval)
                *nval = ent->nval;

        return (const char **) ent->vals;
}

const char *argparse_val(argparse_t *ap, const char *name)
{
        const char **vals;

        vals = argparse_vals(ap, name, NULL);
        if (!vals)
                return NULL;

        return vals[0];
}

const char *argparse_arg(argparse_t *ap)
{
        return ap->arg;
}

const char *argparse_error(void)
{
        return error_buf;
}