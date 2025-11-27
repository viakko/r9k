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

#define OP_NULL   (0)
#define OP_REQVAL (1 << 0) /* required value */
#define OP_CONCAT (1 << 1) /* -O1 -O2 */
#define OP_NOGRP  (1 << 2) /* not allow group */

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
};

struct argparser;

/* If a result doesn't equal to 0 that mean error. */
struct argparser *argparser_create(void);
void argparser_free(struct argparser *ap);

/* Add options to argparser, if no short name set '?' to the shortopt.
 * Default no argument option can merge to one option for short name.
 * Short option group only last one option can accept parameter. */
int argparser_add0(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   uint32_t flags); /* no argument */

int argparser_add1(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   const char *tips,
                   uint32_t flags); /* 1 argument */

int argparser_addn(struct argparser *ap,
                   struct option **pp_option,
                   const char *shortopt,
                   const char *longopt,
                   int max,
                   const char *tips,
                   uint32_t flags); /* n argument */

/* Parsing arguments */
int argparser_run(struct argparser *ap, int argc, char *argv[]);
const char *argparser_error(struct argparser *ap);

/* Get position values */
uint32_t argparser_count(struct argparser *ap);
const char *argparser_val(struct argparser *ap, uint32_t index);

#endif /* ARGPARSER_H_ */