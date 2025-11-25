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
#include <argparser.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>