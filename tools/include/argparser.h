/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 * argparse - Lightweight command-line argument parsing library
 *
 * Provides multi-sytle command parsing with support for both
 * short and long options. Short option support string type,
 * and single-character options can be grouped.
 *
 * The rules:
 *  - If a long option or a short option string includes 'abc', it gets priority
 *    in processing.
 *  - If 'abc' not found in long option, it will be split into single-character
 *    options for short option matching.
 *  - Supports argument specified with either spaces or equal signs.
 *  - When arguments are specified using spaces, multiple values are supported.
 *  - If a value starts with '-', you need to use equal sign accept the value.
 *  - Short option groups not support using the equal sign to accept value.
 */
#ifndef ARGPARSER_H_
#define ARGPARSER_H_

#include <stdint.h>

#define opt_none    (0)
#define opt_reqval  (1 << 0) /* required value */
#define opt_concat  (1 << 1) /* -O1 -O2 */
#define opt_nogroup (1 << 2) /* not allow group */

#define __acb_help __argparser_acb_help
#define __acb_version __argparser_acb_version

struct argparser;
struct option;

/* return zero means success otherwith error. */
typedef int (*fn_argparser_callback)(struct argparser *, struct option *);

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

        /* built-in */
        uint32_t _capacity;
        struct option** _refs;
        fn_argparser_callback _cb;
};

int __argparser_acb_help(struct argparser *ap, struct option *opt);
int __argparser_acb_version(struct argparser *ap, struct option *opt);

/* If a result doesn't equal to 0 that mean error. */
struct argparser *argparser_create(const char *name, const char *version);
void argparser_free(struct argparser *ap);

/* Add options to argparser.
 * Set NULL for unused short or long name, but at least one must be provided.
 * Default no argument option can merge to one option for short name.
 * Short option group only last one option can accept parameter. */
int argparser_add0(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   fn_argparser_callback cb,
                   uint32_t flags); /* no argument */

int argparser_add1(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   fn_argparser_callback cb,
                   uint32_t flags); /* 1 argument */

int argparser_addn(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   fn_argparser_callback cb,
                   uint32_t flags); /* n argument */

/* Parsing arguments */
int argparser_run(struct argparser *ap, int argc, char *argv[]);
const char *argparser_error(struct argparser *ap);
struct option *argparser_find(struct argparser *ap, const char *name);

/* Get position values */
uint32_t argparser_count(struct argparser *ap);
const char *argparser_val(struct argparser *ap, uint32_t index);

/* Get help messsage */
const char *argparser_help(struct argparser *ap);

#endif /* ARGPARSER_H_ */