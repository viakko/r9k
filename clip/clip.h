/*
* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#ifndef CLIP_H_
#define CLIP_H_

typedef void (*PFN_clip_watch)(const char *);

int clip_write(const char *text);
char *clip_read(void);
void clip_watch(PFN_clip_watch watch);

#endif /*  CLIP_H_ */
