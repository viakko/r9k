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
 *      // set std pointer is NULL
 *      argparser_add0(ap, &std, "std", NULL, "switch standard version", NULL, opt_reqval);
 *
 *      // if user input -std, write back using the _refs pointer.
 *      if (argparser_run(ap, argc, argv) != 0)
 *              die(argparser_error(ap));
 *
 *      // check has input -std
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

#define opt_none    (0)
#define opt_reqval  (1 << 0) /* required value */
#define opt_concat  (1 << 1) /* -O1 -O2 */
#define opt_nogroup (1 << 2) /* not allow a group */

#define acb_help argparser_acb_help
#define acb_version argparser_acb_version

typedef struct argparser argparser_t;
typedef struct option option_t;

/* return zero means success otherwith error. */
typedef int (*PFN_argparser_callback)(argparser_t *, option_t *);

struct option
{
        const char*  shortopt;
        const char*  longopt;
        uint8_t      max;
        const char*  tips;
        const char*  sval;
        uint32_t     count;
        const char** vals;
        uint32_t     flags;
};

int argparser_acb_help(argparser_t *ap, option_t *opt);
int argparser_acb_version(argparser_t *ap, option_t *opt);

/* If a result doesn't equal to 0 that mean error. */
argparser_t *argparser_create(const char *name, const char *version);
void argparser_free(argparser_t *ap);

/* Add options to argparser.
 * Set NULL for unused short or long name, but at least one must be provided.
 * Default no argument option can merge to one option for short name.
 * Short option group only last one option can accept parameter. */
int argparser_add0(argparser_t *ap,
                   option_t **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags); /* no argument */

int argparser_add1(argparser_t *ap,
                   option_t **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags); /* 1 argument */

int argparser_addn(argparser_t *ap,
                   option_t **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   PFN_argparser_callback cb,
                   uint32_t flags); /* n argument */

/* Parsing arguments */
int argparser_run(argparser_t *ap, int argc, char *argv[]);
const char *argparser_error(argparser_t *ap);
option_t *argparser_has(argparser_t *ap, const char *name);

/* Get position values */
uint32_t argparser_count(argparser_t *ap);
const char *argparser_val(argparser_t *ap, uint32_t index);

/* Get help messsage */
const char *argparser_help(argparser_t *ap);

#endif /* ARGPARSER_H_ */