/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 viakko
 */
#include "clipboard_macos.h"
#include <Carbon/Carbon.h>

char *__clip_read_macos(void)
{
        static PasteboardRef pb = NULL;

        if (!pb)
                PasteboardCreate(kPasteboardClipboard, &pb);

        PasteboardSynchronize(pb);
        PasteboardItemID item;
        CFDataRef data;
        CFIndex len;
        const char *ptr;
        char *text;

        PasteboardSynchronize(pb);

        if (PasteboardGetItemIdentifier(pb, 1, &item) != noErr)
                return NULL;

        if (PasteboardCopyItemFlavorData(pb, item, CFSTR("public.utf8-plain-text"), &data) != noErr)
                return NULL;

        len = CFDataGetLength(data);
        ptr = (const char *) CFDataGetBytePtr(data);

        text = malloc(len + 1);
        if (text) {
                memcpy(text, ptr, len);
                text[len] = '\0';
        }

        CFRelease(data);

        return text;
}