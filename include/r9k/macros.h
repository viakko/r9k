/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef MACROS_H_
#define MACROS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define die(fmt, ...)                           \
do {                                            \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        exit(EXIT_FAILURE);                     \
} while (0)

#endif /* MACROS_H_ */