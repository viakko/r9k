/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef CLIPBOARD_H_
#define CLIPBOARD_H_

#ifdef __APPLE__
#include "macos/clipboard_macos.h"
#define __clip_read() __clip_read_macos()
#endif

#endif /* CLIPBOARD_H_ */