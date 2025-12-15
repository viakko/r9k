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
 *    Single-char (-O)       | -O 123  | -O=123   | -O123 (O_CONCAT)
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
 *      struct argparser *ap;
 *      struct option    *std;
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
 * NOTE: You don't need to initialize the 'std' option pointer to NULL.
 *       When calling the add function, the libraray automatically sets the
 *       pointer to NULL via the result slot.
 *       If the user provides a corresponding option, the libraray will write
 *       the parsed value back to the pointer through the slot.
 */
#ifndef ARGPARSER_H_
#define ARGPARSER_H_

#include <stdint.h>

#define O_REQUIRED (1 << 1) /* required value */
#define O_CONCAT   (1 << 2) /* allow arguments like: -O1 -O2 */
#define O_NOGROUP  (1 << 3) /* not allow a group */

#define ACB_EXIT_HELP _argparser_builtin_callback_help
#define ACB_EXIT_VERSION _argparser_builtin_callback_version

struct argparser;
struct option;

/* return zero means success otherwith error. */
typedef int (*argparser_callback_t)(struct argparser *, struct option *);

struct option
{
        const char*  shortopt;
        const char*  longopt;
        const char*  tips;
        const char*  sval;
        uint32_t     nval;
        const char** vals;
};

int _argparser_builtin_callback_help(struct argparser *ap, struct option *opt);
int _argparser_builtin_callback_version(struct argparser *ap, struct option *opt);

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
                   argparser_callback_t cb,
                   uint32_t flags); /* no argument */

int argparser_add1(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   argparser_callback_t cb,
                   uint32_t flags); /* 1 argument */

int argparser_addn(struct argparser *ap,
                   struct option **result_slot,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   argparser_callback_t cb,
                   uint32_t flags); /* n argument */

/* Register a new mutually exclusive group.
 * Each call to this macro creates an independent group.
 * Options in the same group cannot appear simultaneously. */
#define argparser_mutual_exclude(ap, ...) \
        _argparser_builtin_mutual_exclude((ap), __VA_ARGS__, NULL)

void _argparser_builtin_mutual_exclude(struct argparser *ap, ...);

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