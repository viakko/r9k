/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

char *readfile(const char *path);
char *readin();

#pragma GCC diagnostic pop
#endif /* IO_UTILS_H_ */