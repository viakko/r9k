//
// Created by Ekko on 2025/11/19.
//
#include <argparse.h>
#include <stdio.h>
#include <string.h>

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
};

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

        for (i = 0; i < ap->nopt; i++) {
                const struct option *op = &ap->opts[i];

                if ((is_short && op->short_name == arg[0])
                    || strcmp(op->long_name, arg) == 0)
                        return i;
        }

        return -1;
}

static void addval(argparse_t *ap, size_t optid, const char *val)
{
        size_t i;
        const struct option *op = NULL;
        struct optent *fo = NULL;

        op = &ap->opts[optid];

        for (i = 0; i < ap->nent; i++) {
                if (ap->ents[i].opt == op) {
                        fo = &ap->ents[i];
                        break;
                }
        }

        if (!fo) {
                struct optent *tmp;

                tmp = realloc(ap->ents, sizeof(*ap->ents) * (ap->nent + 1));
                if (!tmp)
                        return;

                ap->ents = tmp;
                fo = &ap->ents[ap->nent++];
                fo->opt = &ap->opts[optid];
                fo->vals = NULL;
                fo->nval = 0;
                fo->seen = 0;
        }

        fo->seen++;

        if (val && op->has_arg == required_argument) {
                char **tmp = realloc(fo->vals, sizeof(*tmp) * (fo->nval + 1));
                if (!tmp)
                        return;

                fo->vals = tmp;
                fo->vals[fo->nval++] = strdup(val);
        }
}

argparse_t *argparse_parse(const struct option *opts, int argc, char **argv)
{
        ssize_t optid;
        argparse_t *ap;
        const struct option *op;

        ap = calloc(1, sizeof(*ap));
        ap->opts = opts;
        ap->nopt = optcount(opts);

        for (int i = 1; i < argc; i++) {
                optid = optfind(ap, argv[i]);

                if (optid == -1)
                        continue;

                op = &ap->opts[optid];

                char *val = NULL;
                if (op->has_arg == required_argument
                        || op->has_arg == optional_argument) {
                        val = argv[i + 1];
                        ++i;
                }

                addval(ap, optid, val);
        }

        return ap;
}

void argparse_free(struct argparse *ap)
{
        struct optent *op;

        if (!ap)
                return;

        for (size_t i = 0; i < ap->nent; i++) {
                op = &ap->ents[i];

                for (size_t j = 0; j < op->nval; j++)
                        free(op->vals[j]);

                free(op->vals);
        }

        free(ap->ents);
        free(ap);
}