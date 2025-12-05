/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef CLIP_H_
#define CLIP_H_

int clip_write(const char *text);
char *clip_read(void);
void clip_watch(void);

#endif /*  CLIP_H_ */
