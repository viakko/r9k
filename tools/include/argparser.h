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
 *  - If a long option include 'abc', it gets proirity in processing.
 *  - If 'abc' not found in long option, it will be split into single-character
 *    options for short option matching.
 *  - Supports argument specified with either spaces or equal signs.
 *  - When arguments are specified using spaces, multiple values are supported.
 */
#ifndef ARGPARSER_H_
#define ARGPARSER_H_

#include <stdint.h>

struct optunit
{
        const char*  shortopt;
        const char*  longopt;
        uint8_t      max;
        const char*  sval;
        uint32_t     count;
        const char** vals;
};

struct argparser;

/* If a result doesn't equal to 0 that mean error. */
struct argparser *argparser_create(void);
void argparser_free(struct argparser *ap);

/* Add options to argparser, if not short name uses '?'.
 * Default no argument option can merge to one option for short name.
 * Short option group only last one option can accept parameter. */
int argparser_add0(struct argparser *ap, struct optunit **pp_unit, const char *shortopt, const char *longopt, const char *tips); // no argument
int argparser_add1(struct argparser *ap, struct optunit **pp_unit, const char *shortopt, const char *longopt, const char *tips); // 1 argument
int argparser_addn(struct argparser *ap, struct optunit **pp_unit, const char *shortopt, const char *longopt, int max, const char *tips); // n argument

/* Parsing arguments */
int argparser_run(struct argparser *ap, int argc, char *argv[]);

#endif /* ARGPARSER_H_ */