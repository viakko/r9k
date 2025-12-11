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
#ifndef ARGPARSER_H_
#define ARGPARSER_H_

#include <stdint.h>

#define OPT_REQUIRED (1 << 0) /* required value */
#define OPT_CONCAT   (1 << 1) /* -O1 -O2 */
#define OPT_NOGRP    (1 << 2) /* not allow a group */

#define ACB_HELP argparser_acb_help
#define ACB_VERSION argparser_acb_version

struct argparser;
struct option;

/* return zero means success otherwith error. */
typedef int (*PFN_argparser_callback)(struct argparser *, struct option *);

struct option
{
        const char*  shortopt;
        const char*  longopt;
        uint8_t      max;
        const char*  tips;
        const char*  sval;
        uint32_t     nval;
        const char** vals;
};

int argparser_acb_help(struct argparser *ap, struct option *opt);
int argparser_acb_version(struct argparser *ap, struct option *opt);

/* If a result doesn't equal to 0 that mean error. */
struct argparser *argparser_create(const char *name, const char *version);
void argparser_free(struct argparser *ap);

/* Add options to argparser.
 * Set NULL for unused short or long name, but at least one must be provided.
 * Default no argument option can merge to one option for short name.
 * Short option group only last one option can accept parameter. */
int argparser_add0(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags); /* no argument */

int argparser_add1(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags); /* 1 argument */

int argparser_addn(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags); /* n argument */

/* Parsing arguments */
int argparser_run(struct argparser *ap, int argc, char *argv[]);
const char *argparser_error(struct argparser *ap);
struct option *argparser_has(struct argparser *ap, const char *name);

/* Get position values */
uint32_t argparser_count(struct argparser *ap);
const char *argparser_val(struct argparser *ap, uint32_t index);

/* Get help messsage */
const char *argparser_help(struct argparser *ap);

#endif /* ARGPARSER_H_ */