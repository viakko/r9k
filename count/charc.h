/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 *
 */
#ifndef WORD_H_
#define WORD_H_

#include <stdio.h>
#include <r9k/typedefs.h>

size_t charc(const char *str, bool is_utf8);
size_t linec(const char *str);

#endif /* WORD_H_ */