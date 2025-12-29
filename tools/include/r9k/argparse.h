/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 varketh
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
 *      struct argparse *ap;
 *      struct option   *std;
 *
 *      ap = argparse_create("gcc", "1.0");
 *      if (!ap) {
 *              exit(0);
 *      }
 *
 *      // Set std point to NULL.
 *      argparse_add1(ap, &std, "std", "standard", "switch std", "C17|C23", NULL, 0);
 *
 *      if (argparse_run(ap, argc, argv) != 0)
 *              die(argparse_error(ap));
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

/* option flags */
#define O_REQUIRED                           (1 << 1) /* required value */
#define O_CONCAT                             (1 << 2) /* allow arguments like: -O1 -O2 */
#define O_NOGROUP                            (1 << 3) /* not allow a group */

/* error code */
#define A_ERROR_NO_MEMORY                    (  -100) /* allocate memory failed */
#define A_ERROR_NULL_POINTER                 (  -101) /* null pointer */

#define A_ERROR_SYNTAX                       (  -200) /* syntax error */
#define A_ERROR_SYNTAX_UNKNOWN_OPTION        (  -201) /* unknown option */
#define A_ERROR_SYNTAX_INVALID               (  -202) /* invalid option syntax */
#define A_ERROR_SYNTAX_ASSIGNMENT            (  -203) /* invalid assignment syntax */

#define A_ERROR_CONSTRAINT                   (  -300) /* constraint violation */
#define A_ERROR_CONSTRAINT_CONFLICT          (  -301) /* mutual exclusion conflict */
#define A_ERROR_CONSTRAINT_VALUE_LIMIT       (  -302) /* exceeds maximum values */
#define A_ERROR_CONSTRAINT_INVALID_GROUP     (  -303) /* invalid option group */
#define A_ERROR_CONSTRAINT_MULTI_VAL         (  -304) /* multiple value-consuming options in group */
#define A_ERROR_CONSTRAINT_MISSING_VALUE     (  -305) /* option missing required value */
#define A_ERROR_CONSTRAINT_NO_ARGUMENTS      (  -306) /* option does not accept arguments */

#define A_ERROR_RUNTIME                      (  -400) /* runtime error */
#define A_ERROR_RUNTIME_CALLBACK_FAIL        (  -401) /* callback execution failed */

#define A_ERROR_CONFIG                       (  -500) /* configuration error */
#define A_ERROR_CONFIG_NULL_PARSER           (  -501) /* null argparse instance */
#define A_ERROR_CONFIG_NULL_PARENT           (  -502) /* null parent for subcommand */
#define A_ERROR_CONFIG_ALREADY_RUN           (  -503) /* argparse_run already called */
#define A_ERROR_CONFIG_INVALID               (  -504) /* invalid configuration arguments */

struct argparse;
struct option;

/* return zero means success otherwith error. */
typedef int (*argparse_callback_t)(struct argparse *, struct option *);
typedef int (*argparse_register_t)(struct argparse *);
typedef int (*argparse_cmd_callback_t)(struct argparse *);

struct option
{
        const char*  shortopt;          /* short option string, e.g. "-v" */
        const char*  longopt;           /* long option string, e.g. "--verbose" */
        const char*  help;              /* option help message */
        const char*  metavar;           /* value placeholder shown in help output */
        const char*  sval;              /* first value */
        uint32_t     nval;              /* number of values consumed */
        const char** vals;              /* array of consumed values, in parse order */
};

/* If a result doesn't equal to 0 that mean error. */
struct argparse *argparse_create(const char *name, const char *version);
void argparse_destroy(struct argparse *ap);
/* Register subcommand of argparse */
int argparse_cmd(struct argparse *parent, const char *name, const char *desc, argparse_register_t reg, argparse_cmd_callback_t cb);

/* Add options to argparse.
 * Set NULL for unused short or long name, but at least one must be provided.
 * Default no argument option can merge to one option for short name.
 * Short option group only last one option can accept parameter. */
int argparse_add0(struct argparse *ap, struct option **result_slot, const char *shortopt, const char *longopt, const char *help, argparse_callback_t cb, uint32_t flags); /* flag */
int argparse_add1(struct argparse *ap, struct option **result_slot, const char *shortopt, const char *longopt, const char *help, const char *metavar, argparse_callback_t cb, uint32_t flags); /* 1 argument */
int argparse_addn(struct argparse *ap, struct option **result_slot, const char *shortopt, const char *longopt, const char *help, const char *metavar, int maxval, argparse_callback_t cb, uint32_t flags); /* n argument */

/* Register a new mutually exclusive group.
 * Each call to this macro creates an independent group.
 * Options in the same group cannot appear simultaneously. */
#define argparse_mutual_exclude(ap, ...) \
        _argparse_mutual_exclude((ap), __VA_ARGS__, NULL)

void _argparse_mutual_exclude(struct argparse *ap, ...); // NOLINT(*-reserved-identifier)

/* Parsing arguments */
int argparse_run(struct argparse *ap, int argc, char *argv[]);
const char *argparse_error(struct argparse *ap);
struct option *argparse_has(struct argparse *ap, const char *name);

/* Get position values */
uint32_t argparse_count(struct argparse *ap);
const char *argparse_val(struct argparse *ap, uint32_t index);

/* Get help messsage */
const char *argparse_help(struct argparse *ap);

#endif /* ARGPARSER_H_ */