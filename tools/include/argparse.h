/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * argparse - Lightweight command-line argument parsing library
 *
 * Provides argument parsing with support for both
 * short and long options, required/optional arguments, and multi-value options.
 */
#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include <stdlib.h>

#define no_arguments      (0)
#define required_argument (1 << 1)
#define opt_multi         (1 << 2)
#define allow_group       (1 << 3)

struct option
{
        const char short_name;
        const char *long_name;
        unsigned int flags;
        const char *description;
};

typedef struct argparse argparse_t;

argparse_t *argparse_parse(const struct option *opts, int argc, char **argv);
void argparse_free(argparse_t *ap);
int argparse_has(argparse_t *ap, const char *name);
const char *argparse_val(argparse_t *ap, const char *name);
const char **argparse_multi_val(argparse_t *ap, const char *name, size_t *nval);
const char *argparse_arg(argparse_t *ap);
const char *argparse_error(void);

#endif /* ARGPARSE_H_ */
